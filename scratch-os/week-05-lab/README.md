# Week 5 laboratory — memory protection

This laboratory covers week 4's topic, memory management and
protection, and extends the work done in class: the operating system
runs two tasks, each in its own partition of RAM, and you rebuild the
protection that keeps them apart. Part A, in the laboratory, protects
the operating system from the tasks, then the tasks from each other, and
then grants one task a deliberate, read-only view of a window in the
other's partition. Part B, independent work, diagnoses three defective
kernels, adds a third task, establishes what the memory protection does
to device registers, makes data that must not run refuse to run, and
ends with an optional allocator. Hints are in [`HINTS.md`](HINTS.md),
worked solutions in [`SOLUTIONS.md`](SOLUTIONS.md), and the finished
code in [`solution/`](solution/).

## Objectives

On completion you will have:

- decoded a memory-protection fault from its status register and the
  addresses involved, and used the decoding to locate the access that
  caused it;
- protected the kernel's memory from unprivileged tasks with one MPU
  region, and demonstrated with a task of your own what that region does
  not prevent;
- separated the tasks from one another, and stated why the protection of
  the first stage did not;
- granted one task read access to a window in another task's partition,
  with the kernel's checking of system-call pointers agreeing with what
  the hardware permits;
- diagnosed, from the symptom to the cause, three defective kernels;
- established, by prediction and observation, how many tasks one region
  can protect, what the MPU does to an access to a device register, and
  what execute-never prevents;
- (optional) managed a pool of memory inside a task's partition and
  observed fragmentation.

## Starting point

[`starter/`](starter/), a prepared copy of
[`../week-04/solution/`](../week-04/solution/), the finished week 4
operating system. Its `kernel/mpu.c` is complete except for the body of
`mpu_set_task_region`, which has been taken out: exercises A1 to A3
write it. Two tasks, A and B, are supplied, with the demonstration
programs of week 4 (`rogue`, `badptr`, `overflow`) and four more:
`cross`, `share`, `wrap` and `exec`. The linker script defines, in the
top 256 bytes of task A's partition, a window delimited by the symbols
`_shared_start` and `_shared_end`.

Work in `starter/`. Every path below is relative to it. All register and
field names come from `include/cmsdk.h`: keep it open while you work,
and include it in any file of yours that touches a register.

The starter does not run to completion as delivered. Its first run is
the first exercise.

## Hints and solutions

Hints for the exercises that leave the register operations to you are in
[`HINTS.md`](HINTS.md), three levels each, from the concept to the shape
of the code. Worked solutions to every exercise are in
[`SOLUTIONS.md`](SOLUTIONS.md), and the finished code of the exercises
whose result you keep in [`solution/`](solution/). You are expected to
attempt each exercise before consulting either: the failures you meet on
the way are the substance of the work, and a solution read in advance
removes them.

## Structure

Each exercise lists its own sub-steps and a **Checkpoint**, the
observable state that tells you it is done before you move on. Part A
is for the assisted session: it is self-contained, and its three
exercises build on one another. Part B is for work you carry out
independently afterwards.

Three conventions hold for every exercise:

- **State what you expect before you run.** Write down what the program
  should print and where, if anywhere, it should stop. When the result
  differs, you have learned something about your model, not only about
  your code.
- **A first run that fails is part of the exercise.** Where an exercise
  states what a function must achieve and leaves the register
  operations to you, a first attempt may fail. Read the fault report
  before you change anything, decode it, and change one thing at a time.
  Each such exercise ends with a **Debrief**, collapsed, that names what
  failed and why; open it after the checkpoint.
- **Experiments are made in a copy.** B2, B3, B4 and B5 change code you
  do not keep. Make each in a copy of your working folder, with
  `make clean` in it, so that your working folder stays as Part A left
  it.

The demonstration programs are selected at build time with `DEMO=<word>`
on the `make` command line. Each builds into its own folder,
`build/demo-<word>/`, so switching between them needs no `make clean`:

```sh
make DEMO=rogue
make DEMO=cross
```

`make run DEMO=<word>` builds and runs one. QEMU is stopped with
`Ctrl-A` followed by `x`.

The debugger needs two terminals: `make debug` in the first one, then in
the second `arm-none-eabi-gdb build/scratch-os.axf` and
`target remote :1234` (for a demonstration, the image in
`build/demo-<word>/`). How to obtain the debugger is in
[`environment-setup.md`](../../course-material/docs/environment-setup.md).
To read a register by name, stop in a function of a file that includes
`cmsdk.h`: `break MemManage_Handler` and `continue`.

If you are stuck on a sub-step, read the hints of that exercise first,
and its solution after.

---

## Part A — in the laboratory

### A1. The tasks against the operating system

The MPU of week 4 has been configured by `mpu_init`: region 0 lets the
tasks read and execute FLASH, and the MPU is on. `mpu_set_task_region( base, size )`
is called each time a task starts and is meant to describe the memory
that task may use. Its body is empty.

1. Build and run the starter (`make run`). Record what is printed and
   where it stops.
2. Decode the report. Use the table of the status byte in
   `lecture-notes/week-04.md`, "The MemManage fault", and name the bit
   that is set. In the debugger, stop in `MemManage_Handler` and read
   `$psp`; compare it with the symbols that `arm-none-eabi-nm -n
   build/scratch-os.axf` gives for task A's stack. Say, in one
   sentence, which access was refused and why.
3. Before running anything else, write down: the start and end of each
   task's partition; the first address after task B's partition; and
   whether the kernel's own variables, `uptime_ms` among them, lie
   before or after it. Check each with `nm`.
4. Write the body of `mpu_set_task_region` for this stage, using
   `lecture-notes/week-04.md`, "Computing SIZE and RBAR" and "Decoding
   RASR". Behaviour required: region 1 describes the memory of **both**
   task partitions and nothing else, read-write for the tasks and never
   executable, and the function does not depend on which task is
   starting. The kernel is privileged and needs no region of its own.
5. Run the normal program, then `rogue`, then `cross`. Before `cross`,
   write down what you expect. `cross` has task B store a value into the
   first variable of task A, which has already exited, and read it back.

**Checkpoint:** the normal program prints the output of week 4 (task A,
then task B's uptime lines). `rogue` stops in the MemManage report with
the address of `uptime_ms`. You have run `cross` and can say what it
printed and what that shows the region does not do.

<details><summary>Debrief</summary>

An unprivileged access, including the pops of an exception return, is
allowed only if a region grants it. With region 1 empty, popping the
first frame from task A's stack was refused: `CFSR=0x8`, `MUNSTKERR`,
and no address, because the unstacking does not record one.

The register operations that a first attempt may get wrong:

- **Region number and `VALID`.** Writing `MPU->RBAR` with a base alone
  selects whichever region `MPU->RNR` holds, which after reset is
  region 0. The base, `MPU_RBAR_VALID` and the region number written
  together select the region and set its base. A base written without
  them overwrites region 0, the FLASH region, and the first instruction
  fetch of the task is refused: `CFSR=0x1`, before task A prints
  anything.
- **`ENABLE`.** A region programmed without `MPU_RASR_ENABLE` does not
  exist; the symptom is that of the empty body.
- **Size.** The size of a region is a power of two, and the `SIZE`
  field holds its logarithm minus one; `MPU_RASR_SIZE( n )` takes `n`
  and does the subtraction. A region of 4 KB over both partitions covers
  task A only: A runs and B cannot start, with `CFSR=0x8`.
- **Permissions.** Read-only access for the tasks lets the first store
  of task A fail: `CFSR=0x82`, at the address of its first variable.
- **Alignment.** The base of a region is a multiple of its size.

`rogue` stops at `uptime_ms`, because the kernel's variables lie after
the 8 KB region. `cross` does not stop: a region has one set of
permissions, and both partitions are inside it. The operating system is
protected from the tasks. The tasks are not protected from each other.
</details>

### A2. The tasks against each other

Task B's store into task A's memory succeeded in A1 because both
partitions belong to the same region. This exercise changes that.

1. In a comment above the function, or on paper, state why `cross`
   succeeded in A1, in terms of what a region holds.
2. Change `mpu_set_task_region` so that region 1 describes only the
   partition of the task being started, using the `base` and `size` it
   receives. The function is called by `kernel/task.c` when each task
   starts.
3. Run the normal program, `rogue`, `cross` and, before looking at its
   outcome, `share`. Write down where you expect `share` to stop. Its
   outcome is the subject of A3.

**Checkpoint:** `cross` stops in the MemManage report at the address of
task A's first variable, `rogue` still stops at `uptime_ms`, and the
normal program is unchanged. You can say why, with two partitions and
one region each, a store from B into A stops, and what a task still
cannot do that you might want it to do.

<details><summary>Debrief</summary>

A region carries one set of permissions over one range. Two tasks that
must not touch each other's memory need two ranges and therefore two
descriptions of region 1, one per task. They are never in force
together: the kernel reprograms region 1 as each task starts, which is
why the function takes the partition as arguments.

The function is nearly the same as in A1. Its argument is what changed.
`SIZE` is computed from the partition size as the logarithm of a power
of two (`__builtin_ctz( size )` counts the trailing zero bits, which is
that logarithm), so the function works for any task whose partition is
a power-of-two size at an address that is a multiple of it.

`share` stops at the first read of the window, the top 256 bytes of
task A's partition: task B has no access to it.
</details>

### A3. A shared window

Task A owns the window between `_shared_start` and `_shared_end`. This
exercise lets task B read it. In `DEMO=share`, task A writes the text
`hello` into the window before it exits; task B prints the window, asks
the kernel to print it with `sys_write`, then stores into it.

Behaviour required:

1. Task B can read the window. It cannot write it.
2. Task A, the owner, keeps its access to the window as a part of its own
   partition.
3. `sys_write` accepts a buffer inside the window from any task: the
   system call only reads it.
4. Everything else is unchanged.

Use `lecture-notes/week-04.md`, "The pointer check in sys_write_impl"
and "Why the kernel must check pointers". The window is 256 bytes. The
regions of one task are programmed together, by the same function.

Run `DEMO=share`. If it does not behave as the checkpoint says, read the
report and the output before changing any code: the demonstration shows
the three accesses of the exercise one after the other.

**Checkpoint:** `make run DEMO=share` prints `hello` as read by task B;
then `hello` again, printed by `sys_write`, with `sys_write returned 5`;
then the line `B: writing the window` and a MemManage report at the
address `_shared_start`. `make run DEMO=cross`, `DEMO=rogue`,
`DEMO=badptr` and `DEMO=overflow` give what they gave before, and the
normal program prints the output of week 4.

<details><summary>Debrief</summary>

Three things are needed, and each has its own symptom when it is the one
missing.

- **A second region.** Region 1 is the task's partition. The window is a
  second range for tasks other than its owner, so it is region 2: 256
  bytes (`SIZE` for 2^8), base a multiple of 256, read-only for the
  tasks (`MPU_AP_RO`) and never executable. Where two regions overlap,
  the higher-numbered one decides; the window and task B's partition do
  not overlap here, and B3 shows a case that does.
- **Switch it off for the owner.** A region stays programmed until it is
  written again. The owner reaches the window through region 1, so the
  function writes region 2 on every call and disables it for the owner;
  a task started after another would otherwise inherit the region of the
  one before.
- **The kernel's check must agree with the hardware.** `sys_write_impl`
  accepts a buffer only inside the caller's partition or FLASH. With
  region 2 granted and the check unchanged, task B reads the window
  directly and `sys_write` of the same bytes returns -1. The hardware
  and the kernel describe what a task may use in two places, and they
  must say the same.

With region 2 read-write the store to the window succeeds and
`B: window written` is printed: the permission field is what turns a
view into a channel.
</details>

### Done looks like — Part A

- The starter's first fault is decoded and explained.
- `mpu_set_task_region` gives each task a region over its own partition
  and the non-owner a read-only region over the window; the kernel's
  `sys_write` accepts the window.
- The normal program prints the output of week 4. `rogue` and `cross`
  stop in a MemManage report; `share` reads the window, has `sys_write`
  accept it, and stops at the store.
- You can state what A1's single region protected and what it did not.

---

## Part B — independent work

### B1. Three defective kernels

`broken/` holds three replacement files, one defect each. The defect is
a single line. For each, copy `solution/` to a working folder, copy the
replacement over its counterpart, build, and run the program named
below.

| Replacement | Replaces | Run | Symptom |
|---|---|---|---|
| `broken/unaligned-base/mpu.c` | `kernel/mpu.c` | `make run` | task A runs; `starting task B` is printed, then a MemManage report without an address, and task B never prints its greeting |
| `broken/xn-flash/mpu.c` | `kernel/mpu.c` | `make run` | `scratch-os: kernel started`, then `HardFault: HFSR=0x40000000`; no task starts |
| `broken/wrapping-check/syscall.c` | `kernel/syscall.c` | `make run DEMO=wrap` | with the solution, `sys_write returned -1`; here the call returns a positive number and the output is not text |

Diagnose each with the debugger and the emulator's own diagnostics:
`make run-trace` uses `-d unimp`, and `-d guest_errors` is another class
of message that QEMU can print. For each, write, in a comment at the
defect, the cause in one sentence, and compare it with the diagnosis in
`SOLUTIONS.md` afterwards. Copying the corrected file from `solution/`
repairs the program and is not the exercise.

**Checkpoint:** each defect named in a comment, with the register value
or address that proves it.

### B2. A third task

Add a task C that prints one line and returns. It runs after A and
before B, which never returns. Its partition is a third 4 KB partition,
placed after B's. The tasks are declared in `src/tasks.h`, listed in the
table in `kernel/task.c`, and placed by the linker script; use
`lecture-notes/week-04.md`, "Static partitions: the rules" and "The
partition block".

1. Add the task with the per-task regions of A2, and check that all
   three run.
2. In a copy, replace the per-task region by a single region over all
   the task partitions, as in A1. Before running `rogue`, write down the
   base and the size you can use, and whether the region can cover the
   three partitions and nothing else.
3. Run `rogue` with the single region and with the per-task regions.

**Checkpoint:** you can say, with the arithmetic, what one region
over three partitions covers, and what `rogue` does in each
configuration.

<details><summary>Debrief</summary>

Three partitions of 4 KB take 12 KB, which is not a power of two. The
smallest region that covers them is 16 KB, and it extends over the first
4 KB of the kernel's data, which begin where task C's partition ends,
at `0x20003000`. `rogue` then stores into `uptime_ms` without a fault,
and task B's later uptime lines show the kernel's counter overwritten.
With per-task regions `rogue` stops at the address of `uptime_ms`,
`0x20003004`.

A single region protects the kernel only while the tasks' memory
happens to be a power of two at an aligned base. A region per task does
not depend on that.
</details>

### B3. Device registers

In week 4's laboratory an unprivileged application could write the
board's LED register: no MPU region was in force. The MPU is on now.

1. Predict what happens when task B stores to `0x40028000` and reads it
   back, with the per-task regions of A2.
2. Add the store to task B (`src/task_b.c`, after the greeting) and run.
3. Grant task B that register block and no other device: a region
   over the block. Choose its size, its base, its permissions and its
   attributes (`lecture-notes/week-04.md`, "The MPU registers" and
   "Permissions: AP, XN and PRIVDEFENA"). The block is not memory: it
   must not be executable.

**Checkpoint:** your prediction written down; the report of step 2
decoded (which address, which bit); step 3 printing the value read back
from the register.

<details><summary>Debrief</summary>

With the MPU enabled, an unprivileged access that no region covers is
refused, whether the address is memory or a device. The store stops in
the MemManage report with `CFSR=0x82` at `0x40028000`, in A1, A2 and
A3 alike. Granting the register needs a region of its own: the smallest
is 32 bytes, its base a multiple of 32, read-write, never executable,
with the bufferable attribute and not the cacheable one (a device access
must not be held back or served from a copy). The register then reads
back the value stored.

The difference from week 4's laboratory is the MPU, not the privilege
level: in both, the task was unprivileged.
</details>

### B4. Data that must not run

`DEMO=exec` has task B place two instructions in a local array and call
the array as a function.

1. Predict the outcome with the region of A2, and which field of which
   register decides it.
2. Run `make run DEMO=exec`, and decode the report.
3. Clear that field for region 1 in a copy, and run again.
4. Explain why region 1 has the field set and region 0 does not.

**Checkpoint:** both outcomes observed, with the one flag that separates
them named, and the explanation of step 4 written down.

<details><summary>Debrief</summary>

A task's partition is data. With `MPU_RASR_XN` set in region 1, the
fetch of the first instruction in the array is refused: `CFSR=0x1`,
`cause: instruction fetch`. With it cleared the array runs and the call
returns 42. Region 0 holds code, so it must be executable; region 1
holds data, so it is not. A program that can be induced to run its own
data runs whatever an attacker writes there, and the flag removes that
possibility at no cost to a task that only reads and writes.
</details>

### B5. A pool inside a partition (optional)

Week 4's lecture (`lecture-notes/week-04.md`, "Static versus dynamic
allocation") contrasts the two. Write, inside task B, an allocator over
a pool of 1024 bytes in its own partition: `pool_alloc( n )` returns a
block of at least `n` bytes or `0`, and `pool_free( p )` returns it.
Allocate seven blocks of 120 bytes, free the second, fourth and sixth,
and request 256 bytes. Predict the result before running. Then free the
third and request 256 bytes again.

**Checkpoint:** both requests answered as you predicted, with the
amount of memory free at each, and an explanation of the difference.

---

### Done looks like — Part B

- Three defective kernels diagnosed, each defect named at its line.
- You can state the arithmetic that limits one region over three
  partitions.
- You have observed the refusal of a device register and granted one,
  and observed what execute-never prevents.
- (Optional) You have fragmented a pool and seen a request fail with
  enough memory free.

---

## Where this goes next

Week 5 treats scheduling: several tasks, each with its own stack,
taking turns on the processor. This
laboratory has prepared two things it uses.

- **What the kernel must change when the task changes.** Region 1, and
  region 2, describe the running task. A2 and A3 program them when a task
  starts; a kernel that switches among tasks has to do it at every
  switch, and has to switch off a region that the next task must not
  have, as A3 does for the owner of the window.
- **A task that fails alone.** A task that touches memory outside its
  regions is stopped by a fault that names the address. A scheduler that
  runs several tasks relies on that: one task's mistake should not
  corrupt the others, and the report says which access it was.
