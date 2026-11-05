# own-os

A bare-metal educational operating system for the **ARM Cortex-M3**, running on the **ARM MPS2-AN385** platform via QEMU.

Designed and implemented by **Luca Seggiani** (2026), based on some code provided by [Stefano Di Carlo](https://www.polito.it/personale?p=003272), licensed under Creative Commons Attribution-ShareAlike 4.0 International (CC BY-SA 4.0).

## Features

- **Privilege Separation**: Kernel runs privileged using the Main Stack Pointer (`MSP`); user processes run unprivileged in Thread mode using the Process Stack Pointer (`PSP`).
- **Memory Protection (MPU)**: Each process gets an independent 4 KiB RAM partition containing its stack, data, and BSS. Partitions are marked Execute-Never (`XN`). Flash is mapped globally read-only.
- **Preemptive Scheduler**: Priority-based scheduling with three levels (`PRIO_HIGH`, `PRIO_LOW`, `PRIO_IDLE`) and round-robin timeslicing (`QUANTUM = 10 ms`).
- **PendSV Context Switching**: Deferred context switches execute at the lowest exception priority (`0xF0`), keeping time-critical interrupts short.
- **Process Management**: Process state machine with `PROC_RUNNING`, `PROC_READY`, `PROC_BLOCKED`, and `PROC_TERMINATED` states. Runqueues and blocking queues use linked lists.
- **Timer Primitives**:
  - `sleep(ms)` / `delay(dev, ms)`: Block the calling process for a relative duration.
  - `until(dev, ms)`: Block until an absolute timestamp is reached.
  - Timer queues use delta encoding to efficiently track pending wakeups.
- **Blocking Serial I/O**: Interrupt-driven UART receive with process wait queues. Reads block when no data is available and resume when characters arrive.
- **Synchronization Primitives**:
  - **Semaphores**: Kernel-managed counting semaphores with blocking lock operations and FIFO waiter queues.
  - **Mutexes**: Kernel-managed mutual exclusion locks with ownership tracking and blocking wait queues.
  - Both use fixed-size kernel object pools and IRQ-safe critical sections.
- **Cooperative Yielding**: `yield()` allows a process to voluntarily give up the CPU.
- **Hardware Drivers**: Three UARTs, SysTick, two CMSDK APB timers, semihosting, GPIO, and a hardware watchdog.
- **Automated Process Generation**: `make_procs.py` discovers user processes, extracts their priorities, assigns aligned 4 KiB partitions, and generates the process table and linker script.
- **Freestanding C Library**: Minimal user runtime providing formatted I/O, line input, timing, synchronization, and utility functions.

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
|  |   Timer Queues | UART Wait Queues | Semaphore Queues              |  |
|  |   Mutex Wait Queues                                                |  |
|  +-------------------------------------------------------------------+  |
|  | Drivers: UART | SysTick | APB Timers | MPU | Watchdog | Semihost   |
|  +-------------------------------------------------------------------+  |
|                          Uses MSP, Handler Mode                         |
+-------------------------------------------------------------------------+
|               Hardware / QEMU (ARM MPS2-AN385 Cortex-M3)                |
+-------------------------------------------------------------------------+
```

## Memory & MPU Partitioning

The MPU enforces hardware boundaries between kernel memory and user processes.

| Region | Memory | Base Address | Size | Access (Priv / Unpriv) | Executable |
|---|---|---|---|---|---|
| 0 | FLASH | `0x00000000` | 4 MiB | RO / RO | Yes |
| 1 | Process partition | Dynamic (`spart`) | 4 KiB | RW / RW | No (XN) |
| Default map | System memory | `*` | - | Privileged default map | - |

Region 0 contains the vector table, kernel code, user code, and read-only constants.

Region 1 is reconfigured on each context switch to point to the incoming process partition. Each partition contains:

- Stack: 1 KiB.
- Initialized data (`.data`).
- Zero-initialized data (`.bss`).
- Padding up to 4 KiB.

The default privileged memory map provides access to kernel RAM, peripherals, and system control registers. Unprivileged processes cannot access memory outside their configured MPU region or execute from RAM.

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
    uint32_t sp;
    const char *name;
    process_state state;
    process_priority prio;
	process_block_condition block; // conditions when in PROC_BLOCKED
    int (*entry)(void);
    uintptr_t spart, epart;
    uintptr_t estack;
    uintptr_t sidata, sdata, edata;
    uintptr_t sbss, ebss;
    struct process *next;
} process;
```

### Initialization

1. `kernel_main()` initializes fault handlers, UARTs, timers, watchdog, and MPU Region 0.
2. `handler_init()` configures exception priorities:
   - SysTick and SVCall: `0xE0`.
   - PendSV: `0xF0`.
3. `init_processes()` loads the generated process table.
   - Copies initialized data from Flash to RAM.
   - Clears BSS.
   - Initializes the process stack with an exception frame and callee-saved registers.
   - Adds the process to its priority runqueue.
4. `scheduler_next()` selects the highest-priority ready process, configures its MPU partition, and marks it `PROC_RUNNING`.
5. `systick_init(0, 10)` configures the 10 ms system tick.
6. `start_process()` drops privilege and enters the initial process.

### PendSV Context Switching

SysTick and other interrupt handlers request context switches through PendSV rather than switching directly.

The PendSV handler executes:

```asm
PendSV_Handler:
    bl context_out
    bl scheduler
    bl context_in
    bx r0
```

- `context_out()` saves `r4-r11` to the outgoing process's PSP and stores the resulting stack pointer in `current->sp`.
- `scheduler()` requeues the outgoing process if it is still running, selects the next ready process, and configures its MPU partition.
- `context_in()` restores `r4-r11` and the incoming PSP.
- Exception return restores the hardware exception frame (`r0-r3`, `r12`, `lr`, `pc`, `xPSR`).

PendSV runs at the lowest exception priority, allowing higher-priority interrupts to finish before the context switch takes place.

### Scheduling

The scheduler uses three runqueues:

- `high_prio_process_queue`
- `low_prio_process_queue`
- `idle_process_queue`

The highest-priority nonempty queue is always selected first. Processes within the same priority level are scheduled round-robin.

SysTick increments the scheduling quantum. When `QUANTUM` expires, it requests PendSV.

## Blocking Primitives

### Timer Sleep

`sleep()`, `delay()` and `until()` use ordered timer delta queues.

- `delay(dev, ms)` blocks the process for a relative duration.
- `sleep(ms)` uses SysTick as its timer device.
- `until(dev, ms)` blocks until an absolute timer timestamp.

Each queue stores the difference between consecutive deadlines rather than absolute timestamps. On each timer tick, the queue head is advanced and expired processes are moved back to their priority runqueues.

### Blocking Serial I/O

`fread()` and `fgets()` use interrupt-driven UART receive queues.

When no characters are available:

1. The calling process is marked `PROC_BLOCKED`.
2. Its destination buffer is stored in `current->block.rx_buf`.
3. The process is inserted into the UART receive wait queue.
4. The scheduler selects another process.

When a character arrives, the UART RX interrupt:

1. Removes a waiting process from the queue.
2. Writes the received character into its destination buffer.
3. Updates the saved return value.
4. Marks the process `PROC_READY` and enqueues it.

### Semaphores

Semaphores provide counting synchronization between processes.

The kernel maintains a fixed-size pool of semaphore objects (`SEMAPHORE_NUM`). Each semaphore contains:

- A token counter.
- A queue of blocked processes.
- Allocation state.
- A pointer used by the free-object list.

Operations:

- `sem_new(initial)`: Allocates a semaphore with an initial token count.
- `sem_lock(sem)`: Consumes a token if available; otherwise blocks the calling process.
- `sem_unlock(sem)`: Wakes a waiting process or increments the token count.
- `sem_free(sem)`: Releases the semaphore and wakes its remaining waiters.

Waiters are stored in process queues and resumed through the scheduler's normal priority runqueues.

### Mutexes

Mutexes provide mutual exclusion with ownership tracking.

The kernel maintains a fixed-size pool of mutex objects (`MUTEX_NUM`). Each mutex contains:

- An owner process pointer.
- A queue of blocked processes.
- Allocation state.
- A pointer used by the free-object list.

Operations:

- `mutex_new()`: Allocates an unlocked mutex.
- `mutex_lock(mut)`: Acquires an unowned mutex or blocks the calling process.
- `mutex_unlock(mut)`: Releases the mutex if called by its owner. If waiters exist, ownership is transferred to the next waiter.
- `mutex_free(mut)`: Releases the mutex and wakes its remaining waiters.

Mutex ownership is enforced by the kernel: a process cannot unlock a mutex owned by another process.

### Atomicity

Semaphore and mutex operations use `irq_save()` / `irq_restore()` to protect shared kernel state against interrupt-level concurrency.

This prevents races while modifying:

- Free-object lists.
- Token counters.
- Mutex ownership.
- Process wait queues.
- Process states and ready queues.

The synchronization objects themselves reside in kernel memory and are accessed through kernel-managed handles.

### Cooperative Yielding

`yield()` invokes `SYS_YIELD`, moving the calling process to the tail of its priority runqueue and scheduling another ready process.

### Process Termination

When a process returns from its entry function or calls `terminate()`, it invokes `SYS_EXIT_PROC`.

The kernel marks the process `PROC_TERMINATED` and schedules another process without requeueing the terminated one.

### Idle Process

The idle process runs at `PRIO_IDLE` and yields while no higher-priority process is ready. It provides a fallback execution context when all other processes are blocked.

## Hardware Drivers & Peripherals

| Driver | Location | Details |
|---|---|---|
| CMSDK UART | `src/kernel/driver/serial/` | Three UARTs, ring-buffered TX/RX, interrupt-driven receive and blocking wait queues. |
| SysTick | `src/kernel/driver/systick/` | 24-bit core down-counter, scheduling quantum and timer delta queue. |
| CMSDK APB Timers | `src/kernel/driver/timer/` | Two timers clocked at 25 MHz. Timer 0 provides periodic ticks; Timer 1 feeds the watchdog. |
| Watchdog | `src/kernel/watchdog/` | CMSDK watchdog with a 10-second timeout, serviced by Timer 1. |
| Semihosting | `src/kernel/driver/semihost/` | Host debugging through ARM `BKPT 0xAB` calls. |
| GPIO | `src/kernel/driver/gpio/` | CMSDK GPIO interface. |

## Automated Process Generation

The build system discovers and registers user processes automatically.

```text
src/user/process/*.c
        |
        v
    make_procs.py
        |
        +--> Extract PROCESS_PRIORITY(...)
        |
        +--> Generate proc_lscript.ld
        |
        +--> Generate process.h
```

For each process, the generator:

1. Scans `src/user/process/` for C source files.
2. Extracts `PROCESS_PRIORITY(PRIO_*)`, defaulting to `PRIO_LOW`.
3. Generates linker sections for the stack, data, BSS, and partition padding.
4. Assigns each process an aligned 4 KiB partition.
5. Generates the process table and boundary symbol declarations.

Adding a new process therefore requires only a new source file and an entry-point function following the `<name>_main` convention.

## User Runtime & Standard Library

User processes include `<lib.h>` for the available runtime functions.

### Process Management

- `PROCESS_PRIORITY(p)`: Sets the process priority.
- `yield()`: Voluntarily yields the CPU.
- `terminate()`: Terminates the calling process.

### I/O

- `printf(fmt, ...)`: Writes formatted output to UART0.
- `fprintf(dev, fmt, ...)`: Writes formatted output to a selected character device.
- `fwrite(dev, buf, len)`: Writes a byte buffer.
- `fread(dev, buf, len)`: Reads a byte buffer, blocking when RX data is unavailable.
- `fputc()`, `fputs()`, `fputi()`, `fputu()`, `fputx()`: Character, string, and integer output.
- `gets(buf, siz)` / `fgets(dev, buf, siz)`: Line input with echoing, newline handling, and backspace editing.

Supported format specifiers: `%s`, `%d`, `%i`, `%u`, `%x`, `%c`, and `%%`.

### Timing

- `sleep(ms)`: Sleeps using SysTick.
- `delay(dev, ms)`: Sleeps using a selected timer.
- `until(dev, ms)`: Waits for an absolute timer timestamp.

### Synchronization

- `sem_new(initial)`: Creates a counting semaphore.
- `sem_lock(sem)`: Acquires a semaphore token, blocking if necessary.
- `sem_unlock(sem)`: Releases a semaphore token.
- `sem_free(sem)`: Frees a semaphore.
- `mutex_new()`: Creates a mutex.
- `mutex_lock(mut)`: Acquires a mutex, blocking if necessary.
- `mutex_unlock(mut)`: Releases a mutex.
- `mutex_free(mut)`: Frees a mutex.

### Utilities

- `atoi(s)`: Converts a string to a signed integer.
- `atou(s)`: Converts a string to an unsigned integer.

## Getting Started

### Prerequisites

Install the ARM embedded toolchain, QEMU, Python 3, and GDB.

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

Secondary UART output is logged to:

- UART1: `out/uart1.log`
- UART2: `out/uart2.log`

### Debugging with GDB

Start QEMU waiting for a GDB connection:

```bash
make debug
```

Attach GDB from another terminal:

```bash
make attach
```

### Code Formatting

```bash
make format
```

## System Call Reference

System calls execute through `svc #IMM`, using `r0-r2` for arguments and returning values in `r0`.

| ID | Name | User API | Arguments | Return | Description |
|---|---|---|---|---|---|
| `1` | `SYS_WRITE` | `fwrite` | `r0`: device, `r1`: buffer, `r2`: length | Bytes written | Writes to a character device. |
| `2` | `SYS_READ` | `fread` | `r0`: device, `r1`: buffer, `r2`: length | Bytes read | Reads from a character device; may block. |
| `3` | `SYS_TICK` | `tick` | `r0`: device | Milliseconds | Returns elapsed timer ticks. |
| `4` | `SYS_DELAY` | `delay` / `sleep` | `r0`: device, `r1`: milliseconds | `0` | Blocks for a relative duration. |
| `5` | `SYS_UNTIL` | `until` | `r0`: device, `r1`: timestamp | `0` | Blocks until an absolute timestamp. |
| `6` | `SYS_YIELD` | `yield` | None | `0` | Yields the CPU. |
| `64` | `SYS_SEM_NEW` | `sem_new` | `r0`: initial tokens | Semaphore handle | Allocates a semaphore. |
| `65` | `SYS_SEM_LOCK` | `sem_lock` | `r0`: semaphore handle | `0` | Acquires a token or blocks. |
| `66` | `SYS_SEM_UNLOCK` | `sem_unlock` | `r0`: semaphore handle | `0` | Releases a token or wakes a waiter. |
| `67` | `SYS_SEM_FREE` | `sem_free` | `r0`: semaphore handle | `0` | Frees a semaphore. |
| `68` | `SYS_MUTEX_NEW` | `mutex_new` | None | Mutex handle | Allocates a mutex. |
| `69` | `SYS_MUTEX_LOCK` | `mutex_lock` | `r0`: mutex handle | `0` | Acquires a mutex or blocks. |
| `70` | `SYS_MUTEX_UNLOCK` | `mutex_unlock` | `r0`: mutex handle | `0` | Releases a mutex if owned by the caller. |
| `71` | `SYS_MUTEX_FREE` | `mutex_free` | `r0`: mutex handle | `0` | Frees a mutex. |
| `128` | `SYS_START_PROC` | `start_process` | None | `0` | Starts the initial unprivileged process. |
| `129` | `SYS_EXIT_PROC` | `terminate` | None | `0` | Terminates the calling process. |

### Device Identifiers

**Character devices**

| ID | Name | Description |
|---|---|---|
| `1` | `SYS_SERIAL0` | Primary UART0 console |
| `2` | `SYS_SERIAL1` | Secondary UART1 |
| `3` | `SYS_SERIAL2` | Tertiary UART2 |
| `4` | `SYS_SEMIHOST` | ARM semihosting console |

**Timer devices**

| ID | Name | Description |
|---|---|---|
| `1` | `SYS_SYSTICK` | Core SysTick timer |
| `2` | `SYS_TIMER0` | CMSDK APB Timer 0 |
| `3` | `SYS_TIMER1` | CMSDK APB Timer 1 |

## Adding a User Process

Create a new C file in `src/user/process/`:

```c
#include "../include/lib.h"

PROCESS_PRIORITY(PRIO_HIGH)

int process_c_main(void) {
    printf("Process C started\n");

    until(SYS_SYSTICK, 2000);

    for (int i = 0; i < 3; i++) {
        printf("Process C: iteration %d\n", i);
        sleep(500);
    }

    yield();

    return 0;
}
```

Then rebuild:

```bash
make run
```

`make_procs.py` discovers the new source file, extracts its priority, assigns a partition, and adds it to the generated process table.

## Repository Structure

```text
.
├── Makefile
├── .clang-format
├── README.md
├── src
│   ├── syscall.h
│   ├── kernel
│   │   ├── kernel.c
│   │   ├── startup/
│   │   ├── handler/
│   │   ├── memory/
│   │   ├── process/
│   │   ├── pendsv/
│   │   ├── syscall/
│   │   ├── atomic/
│   │   ├── delay/
│   │   ├── control/
│   │   ├── irq/
│   │   ├── wfi/
│   │   ├── watchdog/
│   │   ├── log/
│   │   ├── driver/
│   │   └── include/
│   ├── user
│   │   ├── process/
│   │   │   └── process.h
│   │   ├── lib/
│   │   │   ├── lib_asm.S
│   │   │   ├── spinlock/
│   │   │   ├── stdio/
│   │   │   ├── format/
│   │   │   └── util/
│   │   └── include/
│   │       └── lib.h
│   └── scripts/
│       ├── kern_lscript.ld
│       ├── proc_lscript.ld
│       └── make_procs.py
└── out/
```

## License & Author

Developed by **Luca Seggiani** (2026).

See the repository license for licensing terms.
