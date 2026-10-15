# week-04/base

## Starting point

This folder is where you start in class. It is the finished operating
system from week 3, extended so that it runs two tasks, one after the
other, each on its own stack. It builds and runs from the first minute.
It has no memory partitions and no memory protection yet: you add them
live, following the slides. The finished code is in `../solution/`, with
full line-by-line comments.

## How this folder was prepared

It starts from `week-03/solution/`. These changes were made so that
class time goes to memory, not to plumbing:

- The single application became two tasks, `src/task_a.c` and
  `src/task_b.c`.
- A task table and the code that launches and ends tasks were added.
- `SVC_Handler` now picks the right stack pointer (MSP or PSP) before it
  reads the system-call frame.
- A new system call, `SYS_EXIT`, lets a task end itself.
- The MPU and `MMFAR` registers were added to `include/cmsdk.h`.
- A `DEMO` switch was added to the `Makefile`, so task B can misbehave
  on purpose.
- The linker script uses a flat layout: the partition bounds exist but
  are empty, so every symbol the code needs is already defined.

## What is given, and why

You read this code in the Inspect slides. You do not type it.

- **`kernel/task.c`** holds the task table and four functions:
  `partition_init` (prepares each task's stack from the linker symbols),
  `task_launch` (builds the first stack frame and starts a task),
  `task_start_first` (starts the first task) and `task_exit_current`
  (ends the running task and starts the next one).
- **`kernel/svc_entry.s`** has `SVC_Handler`. It checks which stack was
  in use and hands the right frame to `svc_dispatch`.
- **`kernel/syscall.c`** has `svc_dispatch`, which runs the requested
  service, including the new exit service.
- **`lib/syscalls.s`** has the user stubs, including `task_exit`, which
  calls `sys_exit`.
- **`src/task_a.c` and `src/task_b.c`** are the two tasks. `task_b.c`
  holds the `DEMO` blocks: small pieces of bad behaviour that are built
  only when you ask for them.
- **`include/cmsdk.h`** defines the MPU registers and `MMFAR`.
- **`Makefile`**, **`boot/startup.s`**, the drivers and the rest of
  `kernel/` and `lib/` are as in week 3.

## What you will write in class

Unit 1: nothing. You only observe: sizes and symbol addresses, the
memory map, and the `rogue` and `badptr` demos.

Unit 2: the partition block in `scripts/mps2_m3.ld`. It gives each task
its own aligned piece of RAM, with the stack at the bottom.

Unit 3:

1. `mpu_init` and `mpu_set_task_region` in `kernel/mpu.c`: switch the MPU
   on and give the running task access to its own partition only.
2. The address report in `MemManage_Handler` in `kernel/faults.c`: print
   the address that caused the fault.
3. The pointer check in `sys_write_impl` in `kernel/syscall.c`: refuse
   a pointer that is not inside the caller's memory.

## Build, run, debug, run-trace

Build the image:

```
make
```

Run it under QEMU (quit with `Ctrl-A` then `x`):

```
make run
```

Run it with one of the misbehaving-task demos:

```
make run DEMO=rogue
make run DEMO=badptr
make run DEMO=overflow
```

A demo builds into its own folder, `build/demo-<word>/`, so it never
mixes with the normal build. Use that image with gdb and `nm`, for
example `build/demo-rogue/scratch-os.axf`.

Run it and log every access to a device QEMU does not implement:

```
make run-trace
```

Debug it. Start QEMU paused, waiting for a debugger:

```
make debug
```

Then, from a second terminal, attach GDB:

```
arm-none-eabi-gdb build/scratch-os.axf
```

(on Linux the package is usually called `gdb-multiarch`), and inside
GDB:

```
target remote :1234
```

Remove the generated files, demo builds included, with `make clean`.

The full, finished solution is in the sibling folder, `../solution/`.
