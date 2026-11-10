# own-os

A bare-metal educational operating system for the **ARM Cortex-M3**, running on the **ARM MPS2-AN385** platform via QEMU.

Designed and implemented by **Luca Seggiani** (2026), based on some code provided by [Stefano Di Carlo](https://www.polito.it/personale?p=003272), licensed under Creative Commons Attribution-ShareAlike 4.0 International (CC BY-SA 4.0).

## Features

- **Privilege Separation**: Kernel runs privileged using the Main Stack Pointer (`MSP`); user processes run unprivileged in Thread mode using the Process Stack Pointer (`PSP`).
- **Memory Protection (MPU)**:
  - **Process Isolation**: Each process gets an independent 4 KiB RAM partition containing its stack (1 KiB), data, and BSS, marked Execute-Never (`XN`).
  - **Flash Protection**: Flash (4 MiB) is mapped globally read-only and executable.
  - **Userspace Heap**: A dedicated 4 KiB heap region accessible to user processes.
  - **Shared Memory**: Inter-process shared memory window (`SHARE`) allowing coordinated cross-process data sharing.
- **Dynamic Heap Memory (`malloc` / `free`)**: Segregated-fit block allocator partitioning heap memory into small (64 B), medium (128 B), and large (256 B) blocks with bounds checking, alignment validation, and double-free detection.
- **Preemptive Multilevel Scheduler**: Priority-based scheduling with three levels (`PRIO_HIGH`, `PRIO_LOW`, `PRIO_IDLE`) and round-robin timeslicing (`QUANTUM = 10 ms`).
- **PendSV Context Switching**: Deferred context switches execute at the lowest exception priority (`0xF0`), keeping time-critical interrupts fast.
- **Process Management**: Process state machine with `PROC_RUNNING`, `PROC_READY`, `PROC_BLOCKED`, and `PROC_TERMINATED` states. Runqueues and blocking queues use linked lists.
- **Timer Primitives**:
  - `sleep(ms)` / `delay(dev, ms)`: Block the calling process for a relative duration.
  - `until(dev, ms)`: Block until an absolute timer timestamp is reached.
  - Timer queues use delta encoding to efficiently track pending wakeups without CPU spinning.
- **Blocking Serial I/O**: Interrupt-driven UART receive with process wait queues. Reads block when no data is available and resume when characters arrive.
- **Synchronization Primitives**:
  - **Spinlocks**: User-space atomic test-and-set locks with `ldrex`, `strex`, memory barriers (`dmb`), and cooperative yielding on contention.
  - **Semaphores**: Kernel-managed counting semaphores with blocking wait queues and FIFO token release.
  - **Mutexes**: Kernel-managed mutual exclusion locks with ownership tracking, blocking queues, and owner-only unlock enforcement.
- **Bitwise Hardware Drivers (GPIO & LEDs)**:
  - **CMSDK GPIO**: 4 GPIO controllers (GPIO0–GPIO3) supporting 16 pins each with configurable pin modes (`MODE_INPUT`, `MODE_OUTPUT`), digital write, and digital read.
  - **MPS2 FPGAIO**: Board-level driver controlling 2 user LEDs and 2 user pushbuttons.
- **Cooperative Yielding**: `yield()` allows a process to voluntarily give up the remainder of its timeslice.
- **Hardware Drivers**: Ring-buffered UARTs (ports 0, 1, 2), SysTick timer, dual CMSDK APB timers, semihosting, 4 CMSDK GPIO ports, FPGAIO LEDs/buttons, and a hardware watchdog serviced via Timer 1.
- **Automated Process Generation**: `make_procs.py` discovers user processes, extracts their priorities (`PROCESS_PRIORITY`), assigns aligned 4 KiB partitions, and generates the process table and linker script.
- **Freestanding C Library**: Minimal user runtime providing formatted I/O, line input, timing, synchronization, heap allocation, bitwise pin operations, and utility functions.

## Architecture

```text
+-------------------------------------------------------------------------+
|                        User Space (Unprivileged)                        |
|                                                                         |
|  +--------------------+   +--------------------+   +-----------------+  |
|  |     process_a      |   |     process_b      |   |      idle       |  |
|  |    (PRIO_LOW)      |   |    (PRIO_HIGH)     |   |   (PRIO_IDLE)   |  |
|  | (Thread Mode, PSP) |   | (Thread Mode, PSP) |   |  (Yields / WFI) |  |
|  +---------+----------+   +---------+----------+   +--------+--------+  |
|            |                        |                       |           |
|            +-------------------+----+-----------------------+           |
|                                |                                        |
|                       System Calls (SVC)                                |
+--------------------------------v----------------------------------------+
|                        Kernel Space (Privileged)                        |
|                                                                         |
|  +-------------------------------------------------------------------+  |
|  |                   Priority-Based Scheduler                        |  |
|  |   High Queue | Low Queue | Idle Queue | Active Process            |  |
|  +-------------------------------------------------------------------+  |
|  |                  PendSV Context Switching                         |  |
|  |   context_out() -> scheduler() -> context_in()                    |  |
|  +-------------------------------------------------------------------+  |
|  |                    Blocking Subsystems                            |  |
|  |   Timer Delta Queues | UART Wait Queues | Semaphore Wait Queues   |  |
|  |   Mutex Wait Queues                                                |  |
|  +-------------------------------------------------------------------+  |
|  | Drivers: UART | SysTick | APB Timers | GPIO | FPGAIO | Watchdog    |  |
|  +-------------------------------------------------------------------+  |
|                          Uses MSP, Handler Mode                         |
+-------------------------------------------------------------------------+
|               Hardware / QEMU (ARM MPS2-AN385 Cortex-M3)                |
+-------------------------------------------------------------------------+
```

## Memory & MPU Partitioning

The Cortex-M3 Memory Protection Unit (MPU) enforces hardware boundaries between kernel memory, individual processes, heap space, and shared memory.

| Region | Memory | Base Address | Size | Access (Priv / Unpriv) | Executable | Description |
|---|---|---|---|---|---|---|
| **0** | FLASH | `0x00000000` | 4 MiB | RO / RO | Yes | Vector table, kernel code, user code, constants |
| **1** | Process partition | Dynamic (`spart`) | 4 KiB | RW / RW | No (XN) | Active process stack (1 KiB), data, and BSS |
| **2** | Userspace Heap | `_sheap` | 4 KiB | RW / RW | No (XN) | Shared heap pool for dynamic `malloc`/`free` |
| **3** | Shared Memory | `_sshare` | 4 KiB | RW / RW | No (XN) | Inter-process shared memory (`SHARE` section) |
| **Default** | System Map | `*` | - | Privileged only | - | Kernel RAM (`0x20000000`), CMSDK peripherals |

- **Region 0** is configured statically at boot and covers Flash as read-only and executable.
- **Region 1** is reconfigured on every context switch by `init_partition()` to point to the incoming process's dedicated 4 KiB RAM partition.
- **Region 2** maps the 4 KiB heap section (`.heap`), allowing user processes to read and write dynamically allocated buffers.
- **Region 3** maps the 4 KiB shared section (`.share`), allowing global variables declared with the `SHARE` macro to be accessed across processes.
- **Buffer Safety**: System calls (`fwrite`, `fread`) validate buffer pointers using `check_buf()`, ensuring that user operations cannot read or write outside their assigned partition.

## Heap Memory & Dynamic Allocation

The operating system includes a freestanding segregated-fit memory allocator (`src/user/lib/heap/heap.c`) initialized at boot via `heap_init()` and exposed via `malloc()` and `free()`.

### Heap Layout (`_sheap` to `_eheap`, 4 KiB)

```text
+---------------------------------------+ _sheap (Base)
| heap_control_t (28 B)                 | Control info (free list heads, counters)
+---------------------------------------+ _sheap + 28
| heap_block descriptors (8 B each)     | Descriptor array (start address, next pointer)
+---------------------------------------+
| SMALL Block Pool (64 B blocks)        | Allocations <= 64 bytes
+---------------------------------------+ HEAP_MEDIUM_BOUND (1 KiB)
| MEDIUM Block Pool (128 B blocks)      | Allocations 65 - 128 bytes
+---------------------------------------+ HEAP_LARGE_BOUND (2 KiB)
| LARGE Block Pool (256 B blocks)       | Allocations 129 - 256 bytes
+---------------------------------------+ _eheap (End, 4 KiB)
```

### Allocation & Freeing Semantics

- **`malloc(size)`**:
  - Allocations are routed to the smallest block class that fits:
    - `size <= 64`: **SMALL** block pool.
    - `65 <= size <= 128`: **MEDIUM** block pool.
    - `129 <= size <= 256`: **LARGE** block pool.
    - `size > 256`: Rejected (returns `NULL`).
  - Pops the first available block from the appropriate free list, marks `block->start = 0` (in-use flag), and returns a pointer to the payload.
- **`free(ptr)`**:
  - Validates `ptr` against heap bounds.
  - Uses `recover_block()` to determine the exact descriptor corresponding to `ptr` through arithmetic:
    - Rejects addresses outside the heap (`[user] trying to free() outside heap`).
    - Rejects misaligned pointers (`[user] misaligned free()`).
    - Detects and rejects double-frees (`[user] double free()`).
  - Restores the block descriptor's `start` pointer and pushes it back onto the appropriate free list.

### Shared Memory (`SHARE`)

Global variables can be marked with the `SHARE` attribute (`src/user/include/lib.h`):

```c
#define SHARE __attribute__((section(".share")))

// Example: Inter-process shared synchronization primitive
SHARE semaphore_t my_shared_sem;
```

Variables marked with `SHARE` reside in `.share` in RAM, mapped by MPU Region 3, allowing multiple processes to communicate and synchronize without triggering memory management faults.

## Hardware Drivers & Peripherals

| Driver | Location | Type | Details |
|---|---|---|---|
| **CMSDK UART (0, 1, 2)** | `src/kernel/driver/serial/` | Character | Ring-buffered TX/RX with interrupt-driven receive and blocking wait queues. |
| **CMSDK GPIO (0, 1, 2, 3)** | `src/kernel/driver/gpio/` | Bitwise | 4 independent ports (16 pins each). Configures pin direction (`OUTENABLESET` / `OUTENABLECLR`), output data (`DATAOUT`), and input data (`DATA`). |
| **MPS2 FPGAIO (LEDs & Buttons)** | `src/kernel/driver/led/` | Bitwise | Board-level interface controlling 2 user LEDs (`FPGAIO->LED`) and reading 2 user pushbuttons (`FPGAIO->BUTTON`). |
| **SysTick Timer** | `src/kernel/driver/systick/` | Timer | 24-bit core down-counter driving scheduling quantum (`10 ms`), PendSV requests, and timer delta queue. |
| **CMSDK APB Timers (0, 1)** | `src/kernel/driver/timer/` | Timer | Dual 25 MHz timers. Timer 0 provides 10 ms periodic ticks; Timer 1 runs at 2000 ms and feeds the watchdog. |
| **Hardware Watchdog** | `src/kernel/watchdog/` | System | CMSDK watchdog with a 10-second timeout, unlocked via key `0x1ACCE551` and serviced by Timer 1. |
| **ARM Semihosting** | `src/kernel/driver/semihost/` | Character | Debug console I/O using ARM `BKPT 0xAB` calls (`SHST_WRITEC`, `SHST_READC`). |

### Bitwise Device Abstraction (`bitwise_driver`)

Bitwise devices implement the standard interface defined in `src/kernel/include/driver.h`:

```c
typedef struct {
    void (*init)(void *ctx);
    void (*digital_write)(void *ctx, int idx, int val);
    int (*digital_read)(void *ctx, int idx);
    void (*pin_mode)(void *ctx, int idx, int mode);
    void *ctx;
} bitwise_driver;
```

Pins are configured as `MODE_INPUT` (0) or `MODE_OUTPUT` (1) and manipulated using `digital_write`, `digital_read`, and `pin_mode`.

## Process Lifecycle & Scheduler

Each process is tracked by a `process` descriptor:

```c
typedef enum {
    PROC_RUNNING,
    PROC_READY,
    PROC_BLOCKED,
    PROC_TERMINATED
} process_state;

typedef enum {
    PRIO_HIGH,
    PRIO_LOW,
    PRIO_IDLE
} process_priority;

typedef struct process {
    uint32_t sp;                     // Saved stack pointer (PSP)
    const char *name;                // Process identifier
    process_state state;             // Current execution state
    process_priority prio;           // Scheduling priority
    union {
        unsigned int delta;          // Delta time for timer delay queue
        char *rx_buf;                // Destination buffer for blocked serial read
    } block;
    int (*entry)(void);              // Main entry point
    uintptr_t spart, epart;          // 4 KiB MPU partition bounds
    uintptr_t estack;                // Initial stack pointer
    uintptr_t sidata, sdata, edata;  // Initialized data bounds
    uintptr_t sbss, ebss;            // Zero-initialized BSS bounds
    struct process *next;            // Next pointer in process queue
} process;
```

### Initialization & Boot

1. `kernel_main()` initializes fault handlers, UARTs, APB timers, watchdog, and MPU Region 0 (Flash).
2. `mem_region_set` programs MPU Region 2 (Heap) and Region 3 (Shared Memory).
3. `heap_init()` partitions the userspace heap into block pools.
4. `ksem_init_heap()` and `kmutex_init_heap()` initialize synchronization object pools.
5. `init_processes()` loads the process table:
   - Copies initialized data (`.data`) from Flash to RAM.
   - Zeroes `.bss`.
   - Formats initial exception stack frames on `PSP` (`r0-r3, r12, lr, pc, xpsr`) and callee-saved registers (`r4-r11`).
   - Enqueues processes into their designated priority runqueues (`high_prio_process_queue`, `low_prio_process_queue`, or `idle_process_queue`).
6. `systick_init(0, 10)` starts the 10 ms periodic system tick.
7. `start_process()` triggers `SYS_START_PROC`, drops privileges (`CONTROL[0] = 1`), and begins execution in unprivileged Thread mode.

### PendSV Context Switching

Preemption is decoupled from interrupt handlers via the **PendSV** exception:

1. `SysTick_Handler` fires every 10 ms:
   - Advances time and wakes expired delta-timer tasks.
   - Advances `quantum`. When `quantum >= QUANTUM` (10 ms), resets `quantum = 0` and calls `pendsv_request()` (`SCB->ICSR = SCB_ICSR_PENDSVSET`).
2. PendSV is configured with the lowest exception priority (`0xF0`), guaranteeing that all time-critical ISRs complete before context switching begins.
3. `PendSV_Handler` executes:
   ```asm
   PendSV_Handler:
       bl context_out
       bl scheduler
       bl context_in
       bx r0
   ```
   - `context_out()`: Pushes `r4-r11` to `PSP` and updates `current->sp`.
   - `scheduler()`: Requeues the outgoing process if still runnable, calls `scheduler_next()` (checking High -> Low -> Idle queues), and reprograms MPU Region 1 for the new process.
   - `context_in()`: Pops `r4-r11` from the incoming process's stack, restores `PSP`, and sets `r0 = 0xFFFFFFFD`.
   - `bx r0`: Triggers hardware exception return, unstacking `r0-r3, r12, lr, pc, xPSR`.

## Blocking & Synchronization Primitives

### Timer Sleep (`sleep`, `delay`, `until`)

- `delay(dev, ms)`: Blocks for a relative duration in milliseconds.
- `sleep(ms)`: Delay convenience wrapper using `SYS_SYSTICK`.
- `until(dev, ms)`: Blocks until an absolute timestamp is reached (`delta = ms - driver->tick()`).
- Timer queues use delta encoding: nodes store the difference between consecutive deadlines. When the timer advances, expired tasks are transitioned from `PROC_BLOCKED` to `PROC_READY`.

### Blocking Serial I/O (`fread`, `fgets`)

- When reading from a serial device whose ring buffer is empty, `uart_read()` marks the process `PROC_BLOCKED`, stores the buffer destination in `current->block.rx_buf`, and pushes the task to the UART's `rx_queue`.
- Upon character arrival, `UART0RX_Handler` removes the waiting task, delivers the byte, updates the return value, and transitions the process back to `PROC_READY`.

### Spinlocks (`spinlock`)

User-space spinlocks are implemented using ARM exclusive memory instructions:

```c
void spin_lock(spinlock *lock) {
    for (;;) {
        if (ldrex(lock) != 0)
            yield(); // Yield CPU on contention
        if (strex(lock, 1) == 0)
            break;
    }
    dmb(); // Data memory barrier (acquire)
}

void spin_unlock(spinlock *lock) {
    dmb();     // Data memory barrier (release)
    *lock = 0;
}
```

### Semaphores (`semaphore_t`)

- Kernel-managed counting semaphores allocated from a fixed pool (`SEMAPHORE_NUM = 64`).
- `sem_new(initial)`: Allocates a semaphore with initial token count.
- `sem_lock(sem)`: Decrements token count; if zero, blocks the process in `sem->queue`.
- `sem_unlock(sem)`: Wakes the first waiting process or increments token count.
- `sem_free(sem)`: Wakes all remaining waiters and returns semaphore to the pool.

### Mutexes (`mutex_t`)

- Mutual exclusion locks with strict ownership tracking (`MUTEX_NUM = 64`).
- `mutex_new()`: Allocates an unlocked mutex.
- `mutex_lock(mut)`: Acquires lock or blocks the process in `mut->queue`.
- `mutex_unlock(mut)`: Enforces that **only the owner** can release the mutex. Wakes the next waiter and transfers ownership directly, or clears ownership if no waiters remain.
- `mutex_free(mut)`: Wakes all waiters and recycles the mutex.

## Automated Process Generation

The build system discovers and registers user processes automatically:

```text
src/user/process/*.c
        │
        ▼
    make_procs.py
        │
        ├──► Extract PROCESS_PRIORITY(PRIO_*)  (defaults to PRIO_LOW)
        ├──► Generate src/scripts/proc_lscript.ld
        └──► Generate src/user/process/process.h
```

For each process file:
1. `make_procs.py` discovers all `.c` files in `src/user/process/`.
2. Extracts `PROCESS_PRIORITY(...)` attributes.
3. Generates linker partitions with 4 KiB alignments for stack, data, BSS, and padding.
4. Generates `process.h` defining `init_process_table()` with entry functions (`<name>_main`), priority assignments, and boundary symbols.

## User Runtime & Standard Library

User processes include `<lib.h>` for standard runtime APIs:

### Process Management
- `PROCESS_PRIORITY(p)`: Sets priority (`PRIO_HIGH`, `PRIO_LOW`, `PRIO_IDLE`).
- `yield()`: Voluntarily yields the CPU.
- `terminate()`: Terminates the calling process.

### Memory Allocation
- `malloc(size)`: Allocates dynamic memory from segregated block pools (up to 256 B).
- `free(ptr)`: Returns allocated memory to the heap.
- `SHARE`: Storage class attribute placing global variables in inter-process shared memory.

### Bitwise Hardware I/O (GPIO & LEDs)
- `pin_mode(dev, pin, mode)`: Configures pin mode (`MODE_INPUT`, `MODE_OUTPUT`).
- `digital_write(dev, pin, val)`: Sets output pin or LED state (`1` or `0`).
- `digital_read(dev, pin)`: Reads input pin or button state.

### Character & Line I/O
- `printf(fmt, ...)`: Formatted print to UART0.
- `fprintf(dev, fmt, ...)`: Formatted print to a specific character device.
- `fwrite(dev, buf, len)`: Write raw buffer.
- `fread(dev, buf, len)`: Read raw buffer (blocks if empty).
- `fputc()`, `fputs()`, `fputi()`, `fputu()`, `fputx()`: Character, string, and integer output.
- `gets(buf, siz)` / `fgets(dev, buf, siz)`: Interactive line input with echo and backspace editing.

### Timing
- `sleep(ms)`: Relative sleep using SysTick.
- `delay(dev, ms)`: Relative sleep using a selected timer.
- `until(dev, ms)`: Absolute delay until a specific timestamp.

### Synchronization
- `spin_lock(lock)` / `spin_unlock(lock)`: User-space atomic spinlocks.
- `sem_new()`, `sem_lock()`, `sem_unlock()`, `sem_free()`: Kernel semaphores.
- `mutex_new()`, `mutex_lock()`, `mutex_unlock()`, `mutex_free()`: Kernel mutexes.

### Utilities
- `atoi(s)`: String to signed integer.
- `atou(s)`: String to unsigned integer.

## Getting Started

### Prerequisites

Install the ARM embedded toolchain, QEMU, Python 3, and GDB:

**Ubuntu / Debian:**
```bash
sudo apt update
sudo apt install build-essential gcc-arm-none-eabi libnewlib-arm-none-eabi \
                 qemu-system-arm gdb-multiarch python3 clang-format
```

**Arch Linux:**
```bash
sudo pacman -S arm-none-eabi-gcc arm-none-eabi-newlib \
               qemu-system-arm gdb python clang
```

**macOS:**
```bash
brew install --cask gcc-arm-embedded
brew install qemu python clang-format
```

### Building & Running

```bash
make       # Build out/kernel.axf
make run   # Run in QEMU
```

To exit QEMU, press `Ctrl+A`, then `X`.

Secondary UART logs are written to:
- UART1: `out/uart1.log`
- UART2: `out/uart2.log`

### Debugging with GDB

1. In one terminal, start QEMU with GDB stub:
   ```bash
   make debug
   ```

2. In another terminal, attach GDB:
   ```bash
   make attach
   ```

### Code Formatting

```bash
make format
```

## System Call Reference

System calls execute through `svc #IMM`, passing arguments in `r0-r2` and returning values in `r0`.

| ID | Name | User API | Arguments | Return | Description |
|---|---|---|---|---|---|
| `1` | `SYS_WRITE` | `fwrite` | `r0`: dev, `r1`: buf, `r2`: len | Bytes written | Writes to character device. |
| `2` | `SYS_READ` | `fread` | `r0`: dev, `r1`: buf, `r2`: len | Bytes read | Reads from character device (blocking). |
| `8` | `SYS_DIGITAL_WRITE` | `digital_write` | `r0`: dev, `r1`: pin, `r2`: val | `0` | Writes to a bitwise pin (GPIO / LED). |
| `9` | `SYS_DIGITAL_READ` | `digital_read` | `r0`: dev, `r1`: pin | Pin state | Reads from a bitwise pin (GPIO / Button). |
| `10` | `SYS_PIN_MODE` | `pin_mode` | `r0`: dev, `r1`: pin, `r2`: mode | `0` | Sets pin mode (`MODE_INPUT` / `MODE_OUTPUT`). |
| `16` | `SYS_TICK` | `tick` | `r0`: dev | Milliseconds | Returns elapsed timer ticks. |
| `17` | `SYS_DELAY` | `delay` / `sleep` | `r0`: dev, `r1`: ms | `0` | Blocks for a relative duration. |
| `18` | `SYS_UNTIL` | `until` | `r0`: dev, `r1`: timestamp | `0` | Blocks until an absolute timestamp. |
| `19` | `SYS_YIELD` | `yield` | None | `0` | Yields CPU to next ready process. |
| `64` | `SYS_SEM_NEW` | `sem_new` | `r0`: initial tokens | Handle | Allocates a semaphore. |
| `65` | `SYS_SEM_LOCK` | `sem_lock` | `r0`: handle | `0` | Acquires a token or blocks. |
| `66` | `SYS_SEM_UNLOCK` | `sem_unlock` | `r0`: handle | `0` | Releases a token or wakes a waiter. |
| `67` | `SYS_SEM_FREE` | `sem_free` | `r0`: handle | `0` | Frees a semaphore and wakes waiters. |
| `68` | `SYS_MUTEX_NEW` | `mutex_new` | None | Handle | Allocates an unlocked mutex. |
| `69` | `SYS_MUTEX_LOCK` | `mutex_lock` | `r0`: handle | `0` | Acquires mutex or blocks. |
| `70` | `SYS_MUTEX_UNLOCK` | `mutex_unlock` | `r0`: handle | `0` | Releases mutex (owner only). |
| `71` | `SYS_MUTEX_FREE` | `mutex_free` | `r0`: handle | `0` | Frees a mutex and wakes waiters. |
| `128` | `SYS_START_PROC` | `start_process` | None | `0` | Starts initial unprivileged process. |
| `129` | `SYS_EXIT_PROC` | `terminate` | None | `0` | Terminates calling process. |

### Device Identifiers (`src/syscall.h`)

#### Character Devices
| ID | Name | Description |
|---|---|---|
| `1` | `SYS_SERIAL0` | Primary UART0 console (stdio) |
| `2` | `SYS_SERIAL1` | Secondary UART1 (`out/uart1.log`) |
| `3` | `SYS_SERIAL2` | Tertiary UART2 (`out/uart2.log`) |
| `4` | `SYS_SEMIHOST` | ARM semihosting console |

#### Bitwise Devices (GPIO & LEDs)
| ID | Name | Description |
|---|---|---|
| `0` | `SYS_GPIO0` | CMSDK GPIO Controller 0 (16 pins) |
| `1` | `SYS_GPIO1` | CMSDK GPIO Controller 1 (16 pins) |
| `2` | `SYS_GPIO2` | CMSDK GPIO Controller 2 (16 pins) |
| `3` | `SYS_GPIO3` | CMSDK GPIO Controller 3 (16 pins) |
| `4` | `SYS_FPGAIO` | MPS2 FPGAIO (LEDs 0-1, Pushbuttons 0-1) |

#### Timer Devices
| ID | Name | Description |
|---|---|---|
| `1` | `SYS_SYSTICK` | Core SysTick timer (default for `sleep`) |
| `2` | `SYS_TIMER0` | CMSDK APB Timer 0 |
| `3` | `SYS_TIMER1` | CMSDK APB Timer 1 |

## Adding a User Process

Create a new C file in `src/user/process/` (e.g., `src/user/process/process_c.c`):

```c
#include "../include/lib.h"

int process_c_main(void) {
    printf("Hello from process C!\n");
    yield();
    
    return 0; // Triggers SYS_EXIT_PROC automatically
}
```

Then rebuild and run:

```bash
make run
```

`make_procs.py` discovers the source file, extracts its priority, assigns an MPU partition, and adds it to the generated process table.

## Repository Structure

```text
.
├── Makefile                     # Build rules, QEMU runner, GDB targets
├── .clang-format                # Code formatting configuration (LLVM-based)
├── README.md                    # Project documentation
├── src
│   ├── syscall.h                # System call IDs and peripheral device aliases
│   │
│   ├── kernel                   # Core Kernel
│   │   ├── kernel.c             # Kernel initialization, MPU setup, and boot
│   │   ├── startup/             # Reset handler, ISR vector table, Data/BSS init
│   │   ├── handler/             # Fault handlers (HardFault, MemManage, Bus, Usage)
│   │   ├── memory/              # MPU configuration and dynamic partition switching
│   │   ├── process/             # Multilevel scheduler, runqueues, context_in / context_out
│   │   ├── pendsv/              # PendSV handler & context switch request
│   │   ├── syscall/             # SVC handler, syscall table, buffer safety check
│   │   ├── atomic/              # Synchronization (semaphores, mutexes, ldrex/strex)
│   │   ├── delay/               # Timer delta queue implementation
│   │   ├── control/             # CPU privilege register (CONTROL) helpers
│   │   ├── irq/                 # Interrupt masking (PRIMASK save/restore)
│   │   ├── wfi/                 # Low-power WFI wrapper
│   │   ├── watchdog/            # Hardware watchdog driver
│   │   ├── log/                 # Kernel logger (kprintf, kpanic)
│   │   ├── driver/              # UART, GPIO, LED/FPGAIO, SysTick, APB timers, Semihost
│   │   └── include/             # Kernel header definitions (cmsdk.h, driver.h, etc.)
│   │
│   ├── user                     # User Space
│   │   ├── process/             # User applications (idle.c, process_a.c, process_b.c, ...)
│   │   │   └── process.h        # [Generated] Process table and descriptor headers
│   │   ├── lib/                 # User-space runtime library
│   │   │   ├── lib_asm.S        # Syscall assembly wrappers
│   │   │   ├── heap/            # Segregated-fit heap allocator (malloc, free)
│   │   │   ├── spinlock/        # Atomic spinlocks (spin_lock, spin_unlock)
│   │   │   ├── stdio/           # Formatted and line-buffered I/O (printf, gets)
│   │   │   ├── format/          # Formatted string printers (vformat, lformat)
│   │   │   └── util/            # Integer parsing (atoi, atou)
│   │   └── include/
│   │       └── lib.h            # Public user-space library API
│   │
│   └── scripts/                 # Build & Tooling
│       ├── kern_lscript.ld      # Kernel base linker script (Flash, RAM, Heap, Share)
│       ├── proc_lscript.ld      # [Generated] Process partitions linker script
│       └── make_procs.py        # Automatic process partition code generator
│
└── out/                         # Compilation artifacts and logs
```

## License & Author

Developed by **Luca Seggiani** (2026).

See the repository license for licensing terms.
