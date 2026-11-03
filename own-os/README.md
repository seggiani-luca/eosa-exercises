# own-os

A bare-metal educational operating system for the **ARM Cortex-M3**, running on the **ARM MPS2-AN385** platform via QEMU.

Designed and implemented by **Luca Seggiani** (2026), based on some code by my professor ([Stefano Di Carlo](https://www.polito.it/personale?p=003272), code provided as Creative Commons Attribution-ShareAlike 4.0 International (CC BY-SA 4.0)).

---

## Features

- **Privilege Separation**: Kernel runs privileged with the Main Stack Pointer (`MSP`); user processes run unprivileged in Thread mode using the Process Stack Pointer (`PSP`).
- **Memory Protection (MPU)**: Cortex-M3 MPU isolates each process into an independent 4 KiB partition (stack, data, BSS) marked Execute-Never (`XN`). Flash is mapped globally read-only.
- **Priority-Based Round-Robin Preemptive Scheduler**: Multilevel scheduling with strict priority levels (`PRIO_HIGH`, `PRIO_LOW`, `PRIO_IDLE`) and round-robin timeslicing (`QUANTUM = 10 ms`).
- **PendSV-Driven Context Switching**: Unlocked and configured PendSV at lowest exception priority (`0xF0`) to perform asynchronous, deferred context switches (`context_out` / `context_in`) without delaying time-critical interrupts.
- **Full Context Switching**: Dual-stack context switching saves and restores callee-saved registers (`r4`-`r11`) and hardware exception frames on process stacks (`PSP`).
- **Process State Machine & Blocking Queues**: Supports `PROC_RUNNING`, `PROC_READY`, `PROC_BLOCKED`, and `PROC_TERMINATED` states with linked-list queues for scheduler runqueues, timer deltas, and peripheral I/O.
- **Timer Sleep Primitives (Relative & Absolute)**:
  - `sleep(ms)` / `delay(dev, ms)`: Blocks the calling process for a relative duration in milliseconds.
  - `until(dev, ms)`: Blocks the calling process until an absolute target timestamp (milliseconds) is reached.
  - Both primitives insert tasks into an ordered kernel delta queue, waking them on timer ticks without CPU spinning.
- **Blocking Serial I/O**: `fread` / `uart_read` puts processes into `PROC_BLOCKED` when the receive buffer is empty, resuming execution directly via the UART RX interrupt upon incoming data.
- **Cooperative Yielding**: `yield()` syscall allows processes to voluntarily give up the remainder of their timeslice to other ready tasks.
- **Hardware Drivers**: Ring-buffered interrupt-driven UARTs (ports 0, 1, 2), SysTick timer, dual CMSDK APB timers, semihosting, and a hardware watchdog serviced via Timer 1.
- **Automated Process Generator**: `make_procs.py` scans user process sources, extracts `PROCESS_PRIORITY(...)` attributes, assigns aligned 4 KiB partitions in the linker script, and generates process table headers.
- **Freestanding C Library**: Minimal user runtime providing `printf`, `fprintf`, `fgets`, `gets`, `sleep`, `delay`, `until`, `yield`, `terminate`, and integer conversion helpers.

---

## Architecture & Internals

```
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
|                     System Calls / SysTick Traps                        |
+--------------------------------v----------------------------------------+
|                        Kernel Space (Privileged)                        |
|                                                                         |
|  +-------------------------------------------------------------------+  |
|  |                   Multilevel Priority Scheduler                   |  |
|  |   - High Queue: high_prio_process_queue (PROC_READY)              |  |
|  |   - Low Queue:  low_prio_process_queue  (PROC_READY)              |  |
|  |   - Idle Queue: idle_process_queue      (PROC_READY)              |  |
|  |   - Active Task: current (PROC_RUNNING)                           |  |
|  +-------------------------------------------------------------------+  |
|  |                 Deferred PendSV Context Switching                 |  |
|  |   - SysTick_Handler triggers pendsv_request() on quantum expiry   |  |
|  |   - PendSV_Handler (Lowest Priority, 0xF0):                       |  |
|  |       context_out() [push r4-r11 to PSP]                          |  |
|  |       scheduler()   [select highest-priority ready process]       |  |
|  |       context_in()  [pop  r4-r11 from PSP]                       |  |
|  +-------------------------------------------------------------------+  |
|  |                     Blocking State Subsystems                     |  |
|  |   - Timer Delta Queue (delay.c): systick_queue, timer0/1_queue   |  |
|  |   - Device Wait Queues (serial.c): uart0/1/2.rx_queue            |  |
|  +-------------------------------------------------------------------+  |
|  | Drivers: CMSDK UART (0/1/2) | SysTick | APB Timers | MPU | Watchdog |
|  +-------------------------------------------------------------------+  |
|                          Uses MSP, Handler Mode                         |
+-------------------------------------------------------------------------+
|               Hardware / QEMU (ARM MPS2-AN385 Cortex-M3)                |
+-------------------------------------------------------------------------+
```

### Memory & MPU Partitioning

The Cortex-M3 Memory Protection Unit (MPU) enforces hardware boundaries between the kernel and user processes:

| Region | Memory | Base Address | Size | Access (Priv / Unpriv) | Executable | Description |
|---|---|---|---|---|---|---|
| **0** | FLASH | `0x00000000` | 4 MiB | Read-Only / Read-Only (`MPU_AP_RO_RO`) | Yes | Vector table, kernel code, user code, read-only constants |
| **1** | Process Partition | Dynamic (`spart`) | 4 KiB (`_part_size`) | Read-Write / Read-Write (`MPU_AP_RW_RW`) | No (`XN`) | Active process stack (`1 KiB`), data, and BSS |
| **Default** | System Map | `*` | - | Privileged Only (`MPU_CTRL_PRIVDEFENA`) | - | Kernel RAM (`0x20000000`), CMSDK peripherals (`0x40000000`), system core registers |

When the scheduler switches tasks, it reconfigures MPU Region 1 to point to the incoming process partition via `init_partition()`. Any access outside the 4 KiB boundary or attempts to execute from RAM immediately trigger a `MemManage` fault.

---

### Process Lifecycle & Scheduler

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

#### Initialization & Boot
1. `kernel_main()` configures fault handlers, UARTs, APB timers, watchdog, and MPU Region 0.
2. In `handler_init()`, exception priorities are configured:
   - SysTick & SVCall priority: `0xE0`
   - PendSV priority: `0xF0` (lowest exception priority)
3. `init_processes()` loads the process table generated by `make_procs.py`. For each process:
   - Initial data is copied from Flash to RAM (`Data_Init`), and BSS is zeroed (`Bss_Init`).
   - `init_stack()` formats an initial exception stack frame (`r0-r3`, `r12`, `lr=exit_process`, `pc=entry`, `xpsr=0x01000000`) followed by callee-saved registers (`r4-r11 = 0`), storing the resulting top of stack in `proc->sp`.
   - The process is set to `PROC_READY` and queued into its respective priority runqueue (`high_prio_process_queue`, `low_prio_process_queue`, or `idle_process_queue`).
4. `scheduler_next()` selects the highest-priority ready process as `current`, programs its MPU partition, and sets state to `PROC_RUNNING`.
5. `systick_init(0, 10)` sets up periodic 10 ms timer ticks.
6. `start_process()` triggers `SYS_START_PROC`, switching `CONTROL[0] = 1` (`priv_down`), restoring context via `context_in`, and entering unprivileged thread mode.

#### PendSV & Deferred Context Switching
Context switching is decoupled from hardware ISRs using **PendSV**:
- **Why PendSV?**: In ARM Cortex-M, context switches should not occur inside arbitrary high-priority interrupts (like SysTick or UART ISRs), which would delay interrupt processing and complicate preemption. By configuring PendSV with the lowest priority (`0xF0`), the kernel pends a context switch that executes only after all pending ISRs have completed.
- **SysTick Preemption**:
  1. `SysTick_Handler` fires every 10 ms: increments ticks and calls `timer_queue_advance()` to wake expired delta-delay timers.
  2. Increments the `quantum` counter. When `quantum >= QUANTUM` (10 ms):
  3. Resets `quantum = 0` and calls `pendsv_request()` (`SCB->ICSR = SCB_ICSR_PENDSVSET`).
- **PendSV Handler (`src/kernel/pendsv/pendsv_asm.s`)**:
  ```assembly
  PendSV_Handler:
      bl context_out
      bl scheduler
      bl context_in
      bx r0
  ```
  1. `context_out()`: Saves `r4`-`r11` to `PSP` via `stmdb r0!, {r4-r11}`, saving `r0` into `current->sp`.
  2. `scheduler()`:
     - If `current->state == PROC_RUNNING`, changes state to `PROC_READY` and pushes it back into its priority queue.
     - Calls `scheduler_next()`: inspects `high_prio_process_queue` first, then `low_prio_process_queue`, then `idle_process_queue`.
     - Reconfigures MPU Region 1 (`init_partition`) for the newly selected process and marks it `PROC_RUNNING`.
  3. `context_in()`: Restores `r4`-`r11` from `current->sp` via `ldmia r0!, {r4-r11}`, restores `PSP`, and returns `0xFFFFFFFD` in `r0`.
  4. `bx r0`: Triggers hardware exception return, automatically restoring `r0-r3, r12, lr, pc, xPSR`.

---

### Blocking Primitives

#### 1. Delta-Timer Sleep (`sleep`, `delay`, and `until`)
Processes block cleanly on timer devices using an ordered delta queue (`src/kernel/delay/delay.c`):

- **Relative Delay (`sleep` / `delay`)**:
  - User calls `sleep(ms)` or `delay(dev, ms)` -> triggers `SYS_DELAY`.
  - Kernel calls `timer_queue_push(queue, current, ms)`:
    - Sets `current->state = PROC_BLOCKED`.
    - Inserts the task into the sorted delta queue, where each node stores only the delta time remaining after previous nodes.
    - Calls `scheduler()` to immediately switch to another ready task.
- **Absolute Target Timestamp (`until`)**:
  - User calls `until(dev, ms)` -> triggers `SYS_UNTIL`.
  - Kernel calculates `delta = ms - driver->tick(driver->ctx)`.
  - If `delta > 0`, queues the process with that delta into `timer_queue` and calls `scheduler()`. If target time has already passed, returns immediately without blocking.
- **Waking Up**:
  - On each timer tick, `timer_queue_advance()` subtracts elapsed milliseconds from the queue head.
  - When a task's delta drops to 0, its state is set to `PROC_READY` and it is pushed back into its priority queue (`queue_from_prio(ready->prio)`).

#### 2. Blocking Character Read (`fread` / `uart_read`)
- When a process reads from a serial port with `fread()` or `fgets()`:
  - If characters are present in the ring buffer, they are returned immediately.
  - If the receive buffer is empty, `uart_read()` marks `current->state = PROC_BLOCKED`, stores the target buffer pointer in `current->block.rx_buf`, pushes the process into `uart0.rx_queue`, and invokes `scheduler()`.
- When a byte arrives, `UART0RX_Handler` checks `uart0.rx_queue`:
  - If a process is waiting, it writes the received byte directly to `*proc->block.rx_buf`.
  - Writes `1` into the process's stacked `r0` return register (`((uint32_t*)proc->sp)[8] = 1`).
  - Sets `proc->state = PROC_READY` and enqueues it into `queue_from_prio(proc->prio)`.

#### 3. Cooperative Yielding (`yield`)
- A process voluntarily relinquishes CPU time by calling `yield()`, triggering `SYS_YIELD`.
- The kernel calls `scheduler()`, moving `current` from `PROC_RUNNING` to `PROC_READY` at the tail of its priority queue and immediately dispatching the next ready process.

#### 4. Process Termination (`terminate` / `exit_process`)
- When a process finishes or returns from its entry function, it triggers `SYS_EXIT_PROC` (or calls `terminate()`).
- `cleanup_process()` logs the termination details, marks state `PROC_TERMINATED`, and calls `scheduler()` to proceed with remaining active processes without re-queuing the finished task.

#### 5. Idle Task
- An `idle` task (`src/user/process/idle.c`) is assigned priority `PRIO_IDLE` via `PROCESS_PRIORITY(PRIO_IDLE)`.
- It executes `for(;;) yield();`. When higher-priority tasks are blocked (e.g. sleeping or waiting for input), `idle` runs, yielding control until hardware interrupts awaken waiting tasks.

---

## Hardware Drivers & Peripherals

| Driver | Location | Details |
|---|---|---|
| **CMSDK UART (0, 1, 2)** | `src/kernel/driver/serial/` | Interrupt-driven serial driver using circular ring buffers (`sbuf`). Supports non-blocking buffered TX, interrupt-driven RX with blocking process wait-queues, and baud rate configuration. |
| **SysTick Timer** | `src/kernel/driver/systick/` | 24-bit core down-counter driving preemptive scheduler time slicing (triggers `pendsv_request()` on quantum expiry) and the delta-delay queue (`systick_queue`). |
| **CMSDK APB Timers** | `src/kernel/driver/timer/` | Dual timers clocked from the 25 MHz system clock. Timer 0 provides a 10 ms periodic tick; Timer 1 runs at 2000 ms and feeds the watchdog. |
| **Hardware Watchdog** | `src/kernel/watchdog/` | CMSDK Watchdog configured with a 10-second timeout (`0x1ACCE551` unlock key). Automatically serviced by Timer 1 (`watchdog_feed()`). Expiry triggers an NMI followed by system reset. |
| **Semihosting** | `src/kernel/driver/semihost/` | Host debugging channel utilizing ARM `BKPT 0xAB` calls (`SHST_WRITEC`, `SHST_READC`). |

---

## Automated Process Generation

The build system automatically scans and registers user-space applications:

```
src/user/process/*.c  (e.g., idle.c, process_a.c, process_b.c)
        │
        ▼
[ make_procs.py ]
        │
        ├──► Scans source for PROCESS_PRIORITY(PRIO_*)  (defaults to PRIO_LOW)
        ├──► src/scripts/proc_lscript.ld  (Calculates 4 KiB alignments & section layouts)
        └──► src/user/process/process.h   (Declares extern bounds & init_process_table)
```

At build time:
1. `src/scripts/make_procs.py` discovers all `.c` files in `src/user/process/`.
2. Inspects source text for `PROCESS_PRIORITY(PRIO_HIGH | PRIO_LOW | PRIO_IDLE)`.
3. Generates linker sections for each process containing its stack (`1 KiB`), data, BSS, and padding up to `0x1000` (4 KiB).
4. Produces `process.h` defining `init_process_table()` with entry functions (`<name>_main`), priority assignments, and boundary symbols.

---

## User Runtime & Standard Library

User tasks include `<lib.h>` (`src/user/include/lib.h`) for standard library capabilities:

- **Process Priority Configuration**:
  - `PROCESS_PRIORITY(p)`: Sets priority attribute (`PRIO_HIGH`, `PRIO_LOW`, `PRIO_IDLE`) parsed by `make_procs.py`.
- **I/O & Formatted Output**:
  - `printf(fmt, ...)`: Print to default UART0 console.
  - `fprintf(dev, fmt, ...)`: Print to a specific character device (`SYS_SERIAL0`, `SYS_SERIAL1`, `SYS_SERIAL2`, `SYS_SEMIHOST`).
  - Supported specifiers: `%s` (string), `%d` / `%i` (signed integer), `%u` (unsigned integer), `%x` (hex word with `0x`), `%c` (char), `%%` (literal percent).
  - `fputc(dev, c)`, `fputs(dev, s)`, `fputi(dev, n)`, `fputu(dev, u)`, `fputx(dev, w)`.
- **Interactive Line Reading**:
  - `gets(buf, siz)` / `fgets(dev, buf, siz)`: Line-buffered input supporting character echoing, newline handling (`\r`, `\n`), and backspace editing (`\b`, ASCII 127). Blocks when no input is ready.
- **Timing & Scheduling**:
  - `sleep(ms)`: Blocks calling process for `ms` milliseconds using the SysTick delta queue.
  - `delay(dev, ms)`: Generic relative delay syscall on any supported timer device.
  - `until(dev, ms)`: Blocks calling process until absolute timestamp `ms` is reached.
  - `yield()`: Voluntarily relinquishes the CPU to the next ready process.
  - `terminate()`: Terminates the current process.
- **Utilities**:
  - `atoi(s)`: Convert string to signed integer.
  - `atou(s)`: Convert string to unsigned integer.

---

## Getting Started

### Prerequisites

Install the ARM embedded toolchain, QEMU ARM emulator, Python 3, and GDB.

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
# Build kernel binary (out/kernel.axf)
make

# Run in QEMU
make run
```

To exit QEMU, press `Ctrl+A` then `X`.

> **Note:** Secondary UART outputs are logged to files:
> - UART1: `out/uart1.log`
> - UART2: `out/uart2.log`

### Debugging with GDB

1. In one terminal, start QEMU waiting for a GDB connection:
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

---

## System Call Reference

System calls execute via `svc #IMM` instructions using standard ARM registers (`r0`-`r2` for parameters, return value in `r0`).

| ID | Name | User API | Arguments | Return | Description |
|---|---|---|---|---|---|
| `1` | `SYS_WRITE` | `fwrite` | `r0`: dev<br>`r1`: const char *buf<br>`r2`: unsigned int len | Bytes written | Writes up to `len` bytes to a character device. |
| `2` | `SYS_READ` | `fread` | `r0`: dev<br>`r1`: char *buf<br>`r2`: unsigned int len | Bytes read | Reads up to `len` bytes from a character device. Blocks if RX buffer is empty. |
| `3` | `SYS_TICK` | `tick` | `r0`: dev | Milliseconds | Returns elapsed milliseconds from timer device. |
| `4` | `SYS_DELAY` | `delay` / `sleep` | `r0`: dev<br>`r1`: unsigned int ms | `0` | Blocks calling process in timer delta queue for `ms` relative milliseconds. |
| `5` | `SYS_UNTIL` | `until` | `r0`: dev<br>`r1`: unsigned int ms | `0` | Blocks calling process until timer reaches `ms` absolute milliseconds. |
| `6` | `SYS_YIELD` | `yield` | None | `0` | Cooperatively yields CPU to the next ready process. |
| `128` | `SYS_START_PROC` | `start_process` | None | `0` | Drops privilege to unprivileged Thread Mode and starts initial process. |
| `129` | `SYS_EXIT_PROC` | `terminate` | None | `0` | Terminates active process, releases partition, and reschedules. |

### Device Identifiers (`src/syscall.h`)

#### Character Devices
- `SYS_SERIAL0` (`1`): Primary UART0 interface (stdio console)
- `SYS_SERIAL1` (`2`): Secondary UART1 interface (`out/uart1.log`)
- `SYS_SERIAL2` (`3`): Tertiary UART2 interface (`out/uart2.log`)
- `SYS_SEMIHOST` (`4`): Host ARM Semihosting console

#### Timer Devices
- `SYS_SYSTICK` (`1`): ARM Core SysTick counter (default for `sleep`)
- `SYS_TIMER0` (`2`): CMSDK APB Timer 0
- `SYS_TIMER1` (`3`): CMSDK APB Timer 1

---

## Adding a User Process

Creating a new user application requires only creating a C file:

1. Create a `.c` file in `src/user/process/` (e.g., `src/user/process/process_c.c`):

   ```c
   #include "../include/lib.h"

   // Optional priority: PRIO_HIGH, PRIO_LOW (default), PRIO_IDLE
   PROCESS_PRIORITY(PRIO_HIGH)

   int process_c_main(void) {
       printf("Process C started (High Priority)!\n");

       // Delay until an absolute timestamp (e.g. 2000 ms since boot)
       until(SYS_SYSTICK, 2000);

       for (int i = 0; i < 3; i++) {
           printf("Process C: iteration %d\n", i);
           sleep(500); // Relative delay
       }

       printf("Process C yielding...\n");
       yield();

       printf("Process C exiting.\n");
       return 0; // Triggers SYS_EXIT_PROC automatically
   }
   ```

2. Rebuild and execute:
   ```bash
   make run
   ```

`make_procs.py` automatically parses `PROCESS_PRIORITY`, allocates an MPU partition slot, updates the linker script, and appends the process to the startup queue.

---

## Repository Structure

```
.
├── Makefile                     # Build rules, QEMU runner, GDB targets
├── .clang-format                # Code formatting configuration
├── README.md                    # Project documentation
├── src
│   ├── syscall.h                # System call IDs and peripheral device aliases
│   │
│   ├── kernel                   # Core Kernel
│   │   ├── kernel.c             # Kernel initialization and boot sequence
│   │   ├── startup/             # Reset handler, ISR vector table, Data/BSS init
│   │   ├── handler/             # Fault handlers (HardFault, MemManage, Bus, Usage)
│   │   ├── memory/              # MPU configuration and dynamic partition switching
│   │   ├── process/             # Process tables, runqueues, context_in / context_out
│   │   ├── pendsv/              # PendSV handler & context switch request (pendsv.c, pendsv_asm.s)
│   │   ├── syscall/             # SVC handler, syscall table, and dispatcher
│   │   ├── delay/               # Timer delta queue implementation
│   │   ├── control/             # CPU privilege register (CONTROL) helpers
│   │   ├── irq/                 # Interrupt masking (PRIMASK save/restore)
│   │   ├── wfi/                 # Low-power WFI wrapper
│   │   ├── watchdog/            # Hardware watchdog driver
│   │   ├── log/                 # Kernel logger (kprintf, kpanic)
│   │   ├── driver/              # UART, SysTick, APB timers, Semihosting
│   │   └── include/             # Kernel header definitions (pendsv.h, process.h, etc.)
│   │
│   ├── user                     # User Space
│   │   ├── process/             # User applications (idle.c, process_a.c, process_b.c, ...)
│   │   │   └── process.h        # [Generated] Process table and descriptor headers
│   │   ├── lib/                 # User-space runtime library
│   │   │   ├── lib_asm.S        # Syscall assembly wrappers (fwrite, fread, delay, until, yield, ...)
│   │   │   ├── stdio/           # Buffered I/O (printf, fprintf, gets, fgets)
│   │   │   ├── format/          # Formatted string printers (vformat, lformat)
│   │   │   └── util/            # Integer parsing (atoi, atou)
│   │   └── include/
│   │       └── lib.h            # User-space library API
│   │
│   └── scripts                  # Build & Tooling
│       ├── kern_lscript.ld      # Kernel base linker script
│       ├── proc_lscript.ld      # [Generated] Process partitions linker script
│       └── make_procs.py        # Automatic process partition code generator
│
└── out/                         # Compilation artifacts and logs
```

---

## License & Author

Developed by **Luca Seggiani** (2026).  
Licensed under the repository terms.
