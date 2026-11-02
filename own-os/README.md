# own-os

A bare-metal educational operating system for the **ARM Cortex-M3**, running on the **ARM MPS2-AN385** platform via QEMU.

Designed and implemented by **Luca Seggiani** (2026), based on some code by my professor ([Stefano Di Carlo](https://www.polito.it/personale?p=003272), code provided as Creative Commons Attribution-ShareAlike 4.0 International (CC BY-SA 4.0)).

## Features

- **Privilege Separation**: Kernel runs privileged with the Main Stack Pointer (`MSP`); user processes run unprivileged in Thread mode using the Process Stack Pointer (`PSP`).
- **Memory Protection (MPU)**: Cortex-M3 MPU isolates each process into an independent 4 KiB partition (stack, data, BSS) marked Execute-Never (`XN`). Flash is mapped globally read-only.
- **Preemptive Round-Robin Scheduler**: Time-sliced multitasking driven by SysTick timer interrupts (100 ms quantum) with an `idle` task executing low-power wait-for-interrupt (`wfi`).
- **Full Context Switching**: Dual-stack context switching saves and restores callee-saved registers (`r4`-`r11`) and hardware exception frames on process stacks (`PSP`).
- **Process State Machine & Blocking Queues**: Supports `PROC_RUNNING`, `PROC_READY`, `PROC_BLOCKED`, and `PROC_TERMINATED` states with linked-list queues for scheduler runqueues, timer deltas, and peripheral I/O.
- **Delta-Timer Blocking Sleep**: `sleep(ms)` / `delay(dev, ms)` blocks the calling process inside an ordered kernel delta queue, waking tasks without CPU spin-waiting.
- **Blocking Serial I/O**: `fread` / `uart_read` puts processes into `PROC_BLOCKED` when the receive buffer is empty, resuming execution directly via the UART RX interrupt upon incoming data.
- **Cooperative Yielding**: `yield()` syscall allows processes to voluntarily give up the remainder of their timeslice to other ready tasks.
- **Hardware Drivers**: Ring-buffered interrupt-driven UARTs (ports 0, 1, 2), SysTick timer, dual CMSDK APB timers, semihosting, and a hardware watchdog serviced via Timer 1.
- **Automated Process Generator**: `make_procs.py` scans user process sources, assigns aligned 4 KiB partitions in the linker script, and generates process table headers.
- **Freestanding C Library**: Minimal user runtime providing `printf`, `fprintf`, `fgets`, `gets`, `sleep`, `yield`, `terminate`, and integer conversion helpers.

## Architecture & Internals

```
+-------------------------------------------------------------------------+
|                        User Space (Unprivileged)                        |
|                                                                         |
|  +--------------------+   +--------------------+   +-----------------+  |
|  |     process_a      |   |     process_b      |   |      idle       |  |
|  | (Thread Mode, PSP) |   | (Thread Mode, PSP) |   | (Low-power WFI) |  |
|  +---------+----------+   +---------+----------+   +--------+--------+  |
|            |                        |                       |           |
|            +-------------------+----+-----------------------+           |
|                                |                                        |
|                     System Calls / SysTick Traps                        |
+--------------------------------v----------------------------------------+
|                        Kernel Space (Privileged)                        |
|                                                                         |
|  +-------------------------------------------------------------------+  |
|  |               Round-Robin Preemptive Scheduler                    |  |
|  |   - Runqueue: scheduler_process_queue (PROC_READY)                |  |
|  |   - Context Switch: context_out() [push r4-r11]                   |  |
|  |                     context_in()  [pop  r4-r11]                   |  |
|  |   - Active Task: current (PROC_RUNNING)                           |  |
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

### Process Lifecycle & Scheduler

Each process is tracked by a `process` descriptor:

```c
typedef enum {
    PROC_RUNNING,
    PROC_READY,
    PROC_BLOCKED,
    PROC_TERMINATED
} process_state;

typedef struct process {
    uint32_t sp;                     // Saved stack pointer (PSP)
    const char *name;                // Process identifier
    process_state state;             // Current execution state
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
2. `init_processes()` loads the process table generated by `make_procs.py`. For each process:
   - Initial data is copied from Flash to RAM (`Data_Init`), and BSS is zeroed (`Bss_Init`).
   - `init_stack()` formats an initial exception stack frame (`r0-r3`, `r12`, `lr=exit_process`, `pc=entry`, `xpsr=0x01000000`) followed by callee-saved registers (`r4-r11 = 0`), storing the resulting top of stack in `proc->sp`.
   - The process is set to `PROC_READY` and queued into `scheduler_process_queue`.
3. The first process is popped as `current`, its MPU partition is loaded, and `systick_init(0, 100)` starts periodic 100 ms quantum interrupts.
4. `start_process()` triggers `SYS_START_PROC`, switching `CONTROL[0] = 1` (`priv_down`), restoring context via `context_in`, and jumping to unprivileged thread mode.

#### Context Switching (`context_out` / `context_in`)
Context switching is implemented in assembly (`src/kernel/process/process_asm.S`):
- **`context_out()`**: Reads `PSP`, saves `r4`-`r11` using `stmdb r0!, {r4-r11}`, and updates `current->sp`.
- **`context_in()`**: Loads `current->sp`, pops `r4`-`r11` using `ldmia r0!, {r4-r11}`, updates `PSP`, and sets `r0 = 0xFFFFFFFD` (`EXC_RETURN` to unprivileged Thread Mode using PSP).
- When returning from the handler via `bx r0`, Cortex-M3 hardware automatically restores `r0-r3, r12, lr, pc, xPSR`.

#### Preemption & Round-Robin Scheduling
- Every 100 ms, `SysTick_Handler` fires:
  1. Calls `SysTick_Handler_C()`, updating global ticks and calling `timer_queue_advance()` to wake any sleeping tasks.
  2. Invokes `context_out()` to save the interrupted task's state.
  3. Calls `scheduler()`:
     - If `current->state == PROC_RUNNING`, transitions it to `PROC_READY` and pushes it to the back of `scheduler_process_queue`.
     - Pops the next task from `scheduler_process_queue` as `current`.
     - Programs MPU Region 1 for the new process and sets its state to `PROC_RUNNING`.
  4. Invokes `context_in()` and returns, resuming the scheduled process.

### Blocking Primitives

#### 1. Delta-Timer Sleep (`sleep` / `delay`)
Instead of spinning, processes block cleanly on timer devices:
- A user process calls `sleep(ms)` (aliased to `delay(SYS_SYSTICK, ms)`), triggering `SYS_DELAY`.
- The kernel runs `timer_queue_push()` (`src/kernel/delay/delay.c`):
  - Sets `current->state = PROC_BLOCKED`.
  - Inserts the process into an ordered delta-list where `block.delta` stores the time remaining relative to preceding tasks.
  - Invokes `scheduler()` to immediately switch to another ready task.
- On each timer interrupt (SysTick or APB timers), `timer_queue_advance()` subtracts elapsed milliseconds from the list head. Any process whose delta drops to 0 is transitioned to `PROC_READY` and moved back into `scheduler_process_queue`.

#### 2. Blocking Character Read (`fread` / `uart_read`)
- When a process reads from a serial port with `fread()` or `fgets()`:
  - If characters are present in the internal ring buffer, they are returned immediately.
  - If the receive buffer is empty, `uart_read()` marks `current->state = PROC_BLOCKED`, stores the target buffer in `current->block.rx_buf`, pushes the process into the device's `rx_queue`, and invokes `scheduler()`.
- When a byte arrives, `UART0RX_Handler` checks `uart0.rx_queue`:
  - If a process is waiting, it writes the received byte directly to `*proc->block.rx_buf`.
  - It writes `1` into the process's stacked `r0` return register (`((uint32_t*)proc->sp)[8] = 1`).
  - Sets `proc->state = PROC_READY` and enqueues it into `scheduler_process_queue`.

#### 3. Cooperative Yielding (`yield`)
- A process can voluntarily relinquish the CPU by calling `yield()`, which triggers `SYS_YIELD`.
- The kernel calls `scheduler()`, moving `current` from `PROC_RUNNING` to `PROC_READY` at the back of the queue and switching to the next ready process.

#### 4. Process Termination (`terminate` / `exit_process`)
- When a process finishes or returns from its entry function, it triggers `SYS_EXIT_PROC` (or calls `terminate()`).
- `cleanup_process()` logs the partition statistics, marks the state `PROC_TERMINATED`, and calls `scheduler()` to proceed with remaining active processes without re-queuing the finished task.

#### 5. Idle Task
- An `idle` task (`src/user/process/idle.c`) runs in an infinite `wfi()` loop. When all user processes are blocked (e.g., sleeping or waiting for keyboard input), the scheduler selects `idle`, keeping CPU power consumption minimal while waiting for hardware interrupts.

## Hardware Drivers & Peripherals

| Driver | Location | Details |
|---|---|---|
| **CMSDK UART (0, 1, 2)** | `src/kernel/driver/serial/` | Interrupt-driven serial driver using circular ring buffers (`sbuf`). Supports non-blocking buffered TX, interrupt-driven RX with blocking process wait-queues, and baud rate configuration. |
| **SysTick Timer** | `src/kernel/driver/systick/` | 24-bit core down-counter driving preemptive scheduler time slicing and the delta-delay queue (`systick_queue`). |
| **CMSDK APB Timers** | `src/kernel/driver/timer/` | Dual timers clocked from the 25 MHz system clock. Timer 0 provides a 10 ms periodic tick; Timer 1 runs at 2000 ms and feeds the watchdog. |
| **Hardware Watchdog** | `src/kernel/watchdog/` | CMSDK Watchdog configured with a 10-second timeout (`0x1ACCE551` unlock key). Automatically serviced by Timer 1 (`watchdog_feed()`). Expiry triggers an NMI followed by system reset. |
| **Semihosting** | `src/kernel/driver/semihost/` | Host debugging channel utilizing ARM `BKPT 0xAB` calls (`SHST_WRITEC`, `SHST_READC`). |

## Automated Process Generation

The build system automatically scans and registers user-space applications:

```
src/user/process/*.c  (e.g., idle.c, process_a.c, process_b.c)
        │
        ▼
[ make_procs.py ]
        │
        ├──► src/scripts/proc_lscript.ld  (Calculates 4 KiB alignments & section layouts)
        └──► src/user/process/process.h   (Declares extern bounds & init_process_table)
```

At build time:
1. `src/scripts/make_procs.py` discovers all `.c` files in `src/user/process/`.
2. Generates linker sections for each process containing its stack (`1 KiB`), data, BSS, and padding up to `0x1000` (4 KiB).
3. Produces `process.h` defining `init_process_table()` with entry functions (`<name>_main`) and boundary symbols.

## User Runtime & Standard Library

User tasks include `<lib.h>` (`src/user/include/lib.h`) for standard library capabilities:

- **I/O & Formatted Output**:
  - `printf(fmt, ...)`: Print to default UART0 console.
  - `fprintf(dev, fmt, ...)`: Print to a specific character device (`SYS_SERIAL0`, `SYS_SERIAL1`, `SYS_SERIAL2`, `SYS_SEMIHOST`).
  - Supported specifiers: `%s` (string), `%d` / `%i` (signed integer), `%u` (unsigned integer), `%x` (hex word with `0x`), `%c` (char), `%%` (literal percent).
  - `fputc(dev, c)`, `fputs(dev, s)`, `fputi(dev, n)`, `fputu(dev, u)`, `fputx(dev, w)`.
- **Interactive Line Reading**:
  - `gets(buf, siz)` / `fgets(dev, buf, siz)`: Line-buffered input supporting character echoing, newline handling (`\r`, `\n`), and backspace editing (`\b`, ASCII 127). Blocks when no input is ready.
- **Timing & Scheduling**:
  - `sleep(ms)`: Blocks the current process for `ms` milliseconds using the SysTick delta queue.
  - `delay(dev, ms)`: Generic timer delay syscall on any supported timer device.
  - `yield()`: Voluntarily relinquishes the CPU to the next ready process.
  - `terminate()`: Terminates the current process.
- **Utilities**:
  - `atoi(s)`: Convert string to signed integer.
  - `atou(s)`: Convert string to unsigned integer.

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

## System Call Reference

System calls execute via `svc #IMM` instructions using standard ARM registers (`r0`-`r2` for parameters, return value in `r0`).

| ID | Name | User API | Arguments | Return | Description |
|---|---|---|---|---|---|
| `1` | `SYS_WRITE` | `fwrite` | `r0`: dev<br>`r1`: const char *buf<br>`r2`: unsigned int len | Bytes written | Writes up to `len` bytes to a character device. |
| `2` | `SYS_READ` | `fread` | `r0`: dev<br>`r1`: char *buf<br>`r2`: unsigned int len | Bytes read | Reads up to `len` bytes from a character device. Blocks if RX buffer is empty. |
| `3` | `SYS_TICK` | `tick` | `r0`: dev | Milliseconds | Returns elapsed milliseconds from timer device. |
| `4` | `SYS_DELAY` | `delay` / `sleep` | `r0`: dev<br>`r1`: unsigned int ms | `0` | Blocks calling process in timer delta queue for `ms` milliseconds. |
| `5` | `SYS_YIELD` | `yield` | None | `0` | Cooperatively yields CPU to the next ready process. |
| `128` | `SYS_START_PROC` | `start_process` | None | `0` | Drops privilege to unprivileged Thread Mode and starts initial process. |
| `129` | `SYS_EXIT_PROC` | `terminate` | None | `0` | Terminates active process, releases partition, and reschedules. |

As more primitives are added, I will update this README (or get an agent to do it, see how good they are at READMEs?).

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

## Adding a User Process

Creating a new user application requires only creating a C file:

1. Create a `.c` file in `src/user/process/` (e.g., `src/user/process/process_c.c`):

   ```c
   #include "../include/lib.h"

   int process_c_main(void) {
       printf("Process C started!\n");

       for (int i = 0; i < 5; i++) {
           printf("Process C: iteration %d\n", i);
           sleep(500); // Blocks without consuming CPU cycles
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

`make_procs.py` automatically incorporates the new source, allocates an MPU partition slot, updates the linker script, and appends the process to the startup queue.

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
│   │   ├── syscall/             # SVC handler, syscall table, and dispatcher
│   │   ├── delay/               # Timer delta queue implementation
│   │   ├── control/             # CPU privilege register (CONTROL) helpers
│   │   ├── irq/                 # Interrupt masking (PRIMASK save/restore)
│   │   ├── wfi/                 # Low-power WFI wrapper
│   │   ├── watchdog/            # Hardware watchdog driver
│   │   ├── log/                 # Kernel logger (kprintf)
│   │   ├── driver/              # UART, SysTick, APB timers, Semihosting
│   │   └── include/             # Kernel header definitions
│   │
│   ├── user                     # User Space
│   │   ├── process/             # User applications (idle.c, process_a.c, process_b.c, ...)
│   │   │   └── process.h        # [Generated] Process table and descriptor headers
│   │   ├── lib/                 # User-space runtime library
│   │   │   ├── lib_asm.S        # Syscall assembly wrappers (fwrite, fread, delay, yield, ...)
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

## License & Author

Developed by **Luca Seggiani** (2026).  
Licensed under the repository terms.
