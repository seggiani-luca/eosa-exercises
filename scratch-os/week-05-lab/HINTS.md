# Week 5 laboratory — hints

Hints for the exercises of [`README.md`](README.md), to be read when an
exercise does not yield to your own attempt and before its worked
solution in [`SOLUTIONS.md`](SOLUTIONS.md). Each exercise has three
levels. Open one at a time, and stop at the first that lets you
continue: every level is complete in itself, and none contains the
finished code.

1. **The concept** — what to think about, without naming a register.
2. **The rule** — the register, bit or rule involved.
3. **The shape** — the order of the steps, as an outline to fill in.

An exercise without a section here (B5) has no step that withholds
anything.

---

## Part A

### A1. The tasks against the operating system

<details><summary>Level 1 — the concept</summary>

The first fault is not about the code you are going to write; it is
about the code you have not written. Ask what a task needs to be able to
do the moment it starts, and which of those accesses the hardware would
have to be told about in advance. Then ask what range of addresses has to
be described so that both tasks run and the kernel's variables stay
outside it, and read the addresses from the program rather than from
memory: the linker's symbols give them.

When a first attempt fails, the status register tells you which kind of
access was refused, and the debugger tells you at which address.
</details>

<details><summary>Level 2 — the rule</summary>

A region is a base and a size. The size is a power of two, the base is a
multiple of the size, and the `SIZE` field of `MPU->RASR` holds the
logarithm of the size minus one (`MPU_RASR_SIZE( n )` takes the
logarithm `n` and does the subtraction). `MPU->RBAR` selects the region
when it is written with `MPU_RBAR_VALID` and the region number: without
them the write goes to the region held in `MPU->RNR`, which is region 0
after reset, the FLASH region. `MPU_RASR_ENABLE` switches the region on.
The permissions are `MPU_RASR_AP( MPU_AP_FULL_RW )`, and `MPU_RASR_XN`
keeps data from being run.

Two partitions of 4 KB from `0x20000000` make a region of 8 KB at
`0x20000000`, which is 2^13.
</details>

<details><summary>Level 3 — the shape</summary>

In order: write `MPU->RBAR` with the base of the two partitions, the
`VALID` bit and region number 1. Write `MPU->RASR` with the permission
field, the never-execute bit, the shareable and cacheable attributes the
other region uses, the size field for 8 KB, and the enable bit. Finish
with a data synchronisation barrier and an instruction synchronisation
barrier, as `mpu_init` does, so that the next instruction sees the new
region.
</details>

### A2. The tasks against each other

<details><summary>Level 1 — the concept</summary>

Describe, in words, what the single region of A1 says about task A's
partition and about task B's. If the two descriptions are the same, no
single region can tell the tasks apart. What would have to differ, and
at what moment would the kernel have to make it differ?
</details>

<details><summary>Level 2 — the rule</summary>

The region is reprogrammed each time a task starts, by
`mpu_set_task_region`, and only one task runs at a time. The function
receives the base and the size of the partition of the task being
started. Its size is a power of two; `__builtin_ctz` of a power of two
is its logarithm.
</details>

<details><summary>Level 3 — the shape</summary>

The function of A1 with its two fixed numbers replaced by its two
arguments: the base goes into `MPU->RBAR` with the same `VALID` bit and
region number, and the logarithm of the size goes into the size field.
Nothing else changes.
</details>

### A3. A shared window

<details><summary>Level 1 — the concept</summary>

Two places hold the rule about what a task may touch: the MPU, which
stops an access, and the kernel's check, which decides whether the
kernel will read for a task. Grant the view in one and not the other and
the two disagree. Which direction of disagreement did you find first, and
which tool shows it?

Decide also which task needs the extra permission, and what the other
task, the owner, should be left with.
</details>

<details><summary>Level 2 — the rule</summary>

A task has a second region, number 2, for the window: 256 bytes at
`_shared_start`, so `MPU_RASR_SIZE( 8 )`, read-only (`MPU_AP_RO`) and
never executable. The base of 256 bytes is a multiple of 256 because the
partition is aligned to 4 KB. The owner of the window already has it
through region 1. A region stays programmed until it is written again,
so the function writes region 2 on every call.

In `kernel/syscall.c`, `in_range( p, len, start, end )` tells whether a
buffer lies inside a range; `sys_write_impl` combines three of them.
</details>

<details><summary>Level 3 — the shape</summary>

In `mpu_set_task_region`, after region 1: select region 2 by writing
`MPU->RBAR` with the window's base, `VALID` and the number 2. Decide
whether the task being started contains the window (its partition
spans it): if so, write zero to `MPU->RASR`; if not, write the read-only
description. End with the barriers.

In `sys_write_impl`, add one more range to the three the check already
tests: the window, between `_shared_start` and `_shared_end`.
</details>

---

## Part B

### B1. Three defective kernels

<details><summary>Level 1 — the concept</summary>

A defect is a difference from a program that works; you have one. Start
from the symptom, and work back to the first point where the program
could have gone wrong, not to the point where it stopped. Where does the
first fault report come from, and what was the processor doing a moment
before?
</details>

<details><summary>Level 2 — the rule</summary>

The status register names the kind of fault. The debugger can stop in
the handler and show the backtrace of what led there, and the registers
of the MPU can be read back one region at a time by writing the region
number to `MPU->RNR` first. QEMU can also report requests the program
makes of the emulated machine that the machine declines (`-d
guest_errors`). Compare each region with the numbers that the rules of
A1 require. For the last defect, ask what happens to the sum of two
unsigned numbers near the top of the range.
</details>

<details><summary>Level 3 — the shape</summary>

For each: reproduce, stop in the fault handler, read the status
register, then read back the region that the kernel should have
programmed and compare its base and size with the partition. For the
HardFault, find which function was running when the first fault was
raised, and which handler the processor then could not run. For the
check, write down the three numbers (pointer, length, end of range)
and compute the sum by hand.
</details>

### B2. A third task

<details><summary>Level 1 — the concept</summary>

List what the existing tasks have that a new one needs: code, a
partition, a place in the table the kernel reads, a declaration. Then
compute how much memory three partitions cover and ask whether one
region can describe exactly that.
</details>

<details><summary>Level 2 — the rule</summary>

The task table is the array in `kernel/task.c`; the partitions come from
the linker script, where task B's three sections are the model to
repeat; the tasks run in table order. A region's size is a power of two
and its base a multiple of its size. The smallest power of two above
12 KB is 16 KB.
</details>

<details><summary>Level 3 — the shape</summary>

Copy task B's linker block with the names changed, add the new symbols
to the externs and the table in `kernel/task.c` between the entries of A
and B, declare the function in `src/tasks.h`, and add the source file.
For the experiment, replace the two arguments of the region by constants
that describe the smallest region covering all three partitions, and
find what lies in its last 4 KB.
</details>

### B3. Device registers

<details><summary>Level 1 — the concept</summary>

Week 4's laboratory stored to this register from an unprivileged task.
What was different about the machine then? Predict by asking which
description of the address the hardware now holds.
</details>

<details><summary>Level 2 — the rule</summary>

An unprivileged access to an address that no region covers is refused.
The smallest region is 32 bytes; the base is a multiple of the size;
a device is not executable, and it takes the bufferable attribute and
not the cacheable one. Region 3 is free.
</details>

<details><summary>Level 3 — the shape</summary>

In `mpu_set_task_region`, after region 2: select region 3, with the
register block's base; write the description with the read-write
permission, never-execute, the bufferable attribute, the size field for
32 bytes and the enable bit; finish with the barriers.
</details>

### B4. Data that must not run

<details><summary>Level 1 — the concept</summary>

The program places bytes in memory and then transfers control to them.
From the point of view of the hardware that is an instruction fetch from
an address in a data region. Which attribute of a region answers the
question whether instructions may be fetched from it?
</details>

<details><summary>Level 2 — the rule</summary>

The attribute is the execute-never bit of `MPU->RASR`, `MPU_RASR_XN`. A
refused fetch is reported as an instruction-fetch violation in the
MemManage status byte, and the report prints no address for it.
</details>

<details><summary>Level 3 — the shape</summary>

Remove the never-execute bit from the description of region 1 in a copy
of the kernel, rebuild the demonstration, and run it again. Restore the
bit afterwards.
</details>
