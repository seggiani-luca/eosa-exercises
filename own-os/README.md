# own-os

A bare-metal educational OS for the ARM Cortex-M3, running on the ARM MPS2-AN385 platform via QEMU.

## Features

- **Privilege separation**: Kernel runs privileged with the Main Stack Pointer (`MSP`); user processes run unprivileged in Thread mode using the Process Stack Pointer (`PSP`).
- **Memory protection**: Cortex-M3 MPU isolates each process into a 4 KiB partition (stack, data, bss) marked Execute-Never (`XN`). Flash is mapped read-only.
- **System calls**: Traps to the kernel via ARM `SVC` instructions (`SYS_WRITE`, `SYS_READ`, `SYS_TICK`, etc.).
- **Peripherals & Drivers**: Ring-buffered, interrupt-driven UARTs (ports 0, 1, 2), SysTick timer, dual CMSDK APB timers, and a hardware watchdog serviced via Timer 1.
- **Process generation**: `make_procs.py` scans user processes, assigns 4 KiB memory partitions, and generates the linker script and process table.
- **Freestanding C library**: Minimal user runtime providing `printf`, `fprintf`, `fgets`, `sleep`, and integer parsing helpers.

## Architecture

### Memory & MPU

The MPU partitions memory to protect the kernel and isolate processes:

| Region | Memory | Address Range | Size | Access | Description |
|---|---|---|---|---|---|
| **0** | FLASH | `0x00000000` | 4 MiB | Priv RO / Unpriv RO (Executable) | Vector table, kernel & user code |
| **1** | Process Partition | Dynamic (`spart`) | 4 KiB | Priv RW / Unpriv RW (XN) | Active process stack (1 KiB), data, and bss |
| **Default** | System / Peripherals | `*` | - | Privileged only | Kernel RAM (`0x20000000`), peripherals (`0x40000000`) |

Access violations (like user code writing to Flash or touching another partition) trigger a `MemManage` fault.

### Process Lifecycle & System Calls

1. **Boot**: The kernel configures fault handlers, timers, serial ports, the watchdog, and MPU Region 0.
2. **Start Process**: The kernel initializes process `.data`/`.bss`, programs MPU Region 1 to the process partition, builds an initial exception frame on `PSP`, drops privileges (`CONTROL[0] = 1`), and switches to user mode with `EXC_RETURN`.
3. **Syscalls**: User space executes `svc #IMM`. `SVC_Handler` reads arguments from the stacked frame (`r0`-`r2`), looks up the handler in `syscall_table[]`, and writes the result back into the stacked `r0`.
4. **Exit**: When the process calls `SYS_EXIT_PROC` (or returns from its entry function), the kernel disables MPU Region 1, restores privileged mode, and continues kernel execution.

## Getting Started

### Prerequisites

- ARM GNU Toolchain (`arm-none-eabi-gcc`, `arm-none-eabi-newlib`)
- QEMU (`qemu-system-arm`)
- Python 3
- GDB (`gdb-multiarch` or `arm-none-eabi-gdb`)
- `clang-format` (optional, for code formatting)

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
make          # Build out/kernel.axf
make run      # Run in QEMU (press Ctrl+A then X to quit)
```

UART1 and UART2 outputs are logged to `out/uart1.log` and `out/uart2.log`.

### Debugging with GDB

1. Start QEMU in debug mode (listens on port 1234):
   ```bash
   make debug
   ```

2. Attach GDB in another terminal:
   ```bash
   make attach
   ```

### Code Formatting

```bash
make format
```

## System Call Reference

Syscalls use standard ARM calling conventions (`r0`-`r2` for arguments, return value in `r0`).

| ID | Name | Arguments | Return | Description |
|---|---|---|---|---|
| `1` | `SYS_WRITE` | `r0`: dev, `r1`: buf, `r2`: len | Bytes written | Write to character device |
| `2` | `SYS_READ` | `r0`: dev, `r1`: buf, `r2`: len | Bytes read | Read from character device |
| `3` | `SYS_TICK` | `r0`: dev | Milliseconds | Elapsed time from timer device |
| `128` | `SYS_START_PROC` | `r0`: proc | `0` | Configure MPU and enter process |
| `129` | `SYS_EXIT_PROC` | None | `0` | Exit process and restore kernel mode |

### Device Identifiers (`src/syscall.h`)

- **Character Devices**:
  - `SYS_SERIAL0` (1): Primary UART (stdio)
  - `SYS_SERIAL1` (2): Secondary UART (`out/uart1.log`)
  - `SYS_SERIAL2` (3): Tertiary UART (`out/uart2.log`)
  - `SYS_SEMIHOST` (4): ARM Semihosting host console
- **Timer Devices**:
  - `SYS_SYSTICK` (1): Core SysTick counter
  - `SYS_TIMER0` (2): APB Timer 0
  - `SYS_TIMER1` (3): APB Timer 1

As new systemcalls are added I will update this README (or get an agent to do it).

## Adding a User Process

1. Create a `.c` file in `src/user/process/` (e.g. `src/user/process/process_c.c`):

   ```c
   #include "../include/lib.h"

   int process_c_main(void) {
       printf("Hello from Process C!\n");

       for (int i = 0; i < 5; i++) {
           printf("Iteration: %d\n", i);
           sleep(500);
       }

       return 0;
   }
   ```

2. Rebuild and run:
   ```bash
   make run
   ```

`make_procs.py` automatically allocates a 4 KiB partition in the linker script and registers the process in `process.h`.

## Project Structure

```
├── Makefile
├── src
│   ├── syscall.h          # Syscall IDs and device aliases
│   ├── kernel
│   │   ├── kernel.c       # Boot sequence and process runner
│   │   ├── startup/       # Reset handler and vector table
│   │   ├── handler/       # Fault handlers (HardFault, MemManage, Bus, Usage)
│   │   ├── memory/        # MPU configuration
│   │   ├── process/       # Process switching and context setup
│   │   ├── syscall/       # SVC dispatcher
│   │   ├── watchdog/      # Hardware watchdog
│   │   └── driver/        # UART, SysTick, APB timers, semihosting
│   ├── user
│   │   ├── process/       # User applications (process_a.c, process_b.c, ...)
│   │   └── lib/           # stdio, string formatting, sleep, util
│   └── scripts
│       ├── kern_lscript.ld # Kernel linker script
│       └── make_procs.py   # Process partition and header generator
└── out/                   # Build artifacts and logs
```
