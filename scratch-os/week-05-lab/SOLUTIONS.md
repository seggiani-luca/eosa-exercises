# Week 5 laboratory — worked solutions

Solutions to every exercise are provided here. You are expected to
attempt an exercise before consulting its solution, and to read the
hints in [`HINTS.md`](HINTS.md) before this file: the values an
exercise asks you to observe are the substance of the work, and reading
an observed value does not substitute for taking one.

Where an exercise leaves the register operations to you, its solution
follows the course of the work: the first attempts that the exercise
text leads to, what each does, how the cause is found, and then the
working code. The symptoms were produced by running those attempts.

Task document: [`README.md`](README.md). Finished code of the exercises
whose result you keep: [`solution/`](solution/). It holds A1 to A3 in
their final form: A3's `mpu_set_task_region` and `sys_write_impl`. A1
and A2 are the stages on the way to it, and are given below as the
function each leaves behind. B2, B3, B4 and B5 are experiments you make
in a copy; each is given below as the change to apply to `solution/`.
The defective kernels of B1 are in [`broken/`](broken/), one replacement
file each.

Every value, output and listing below was produced by executing the
stated command using the course toolchain (Arm GNU Toolchain 15.3,
QEMU 11.1): against `starter/` with the stated code, or against
`solution/`, or, for an experiment, against `solution/` with the stated
change applied. Each observed value is also classified by what fixes it:

- **architectural** — the ARMv7-M architecture defines it; any
  Cortex-M3 produces it;
- **this build** — it depends on the code the compiler generated, on the
  linker's placement or on how the emulator behaves, so your value may
  differ; the relation the exercise asks about holds regardless.

---

## Part A

### A1. The tasks against the operating system

**The starter's first run.** `make run` prints

```text
scratch-os: kernel started
starting task A
MemManage: CFSR=0x8
```

and stops. There is no address line and no cause line. In gdb, with
`break MemManage_Handler` and `continue`, `p/x SCB->CFSR` gives `0x8`
(architectural) and `p/x $psp` gives `0x200003e0` (this build). Bit 3 of
the MemManage status byte is `MUNSTKERR`: the pop of the frame on
exception return failed. The pop is the access that was refused, and the
frame is on task A's stack, 8 words (32 bytes) below its top:
`arm-none-eabi-nm -n build/scratch-os.axf` gives `_task_a_stack_top` as
`20000400`, and `0x20000400 - 0x20 = 0x200003e0`. With no region
describing it, an unprivileged task has no access to that memory. The
unstacking records no address, so the report has none.

**The layout.** From the linker script and `nm`:

```text
20000000 B _task_a_part_start
20000400 B _task_a_stack_top
20000400 d a_lines
20000f00 A _shared_start
20001000 B _task_a_part_end
20001000 B _task_b_part_start
20001400 B _task_b_stack_top
20001400 d b_seconds
20002000 B _task_b_part_end
20002004 B uptime_ms
```

(this build). Each partition is 4 KB, the stack is the lowest 1 KB of
it, and the first variable follows the stack. The kernel's variables
begin where task B's partition ends, `0x20002000`.

**First attempts.** Each was run in `starter/` with only the body of
`mpu_set_task_region` replaced.

- *`RBAR` written with the base only.* The output stops after
  `starting task A` with `MemManage: CFSR=0x1` and `cause: instruction
  fetch`. Without `VALID` and the region number the write went to the
  region that `MPU->RNR` holds, which is region 0, the FLASH region.
  Its description was replaced by the task's, and the next instruction
  fetch of the task, from FLASH, was refused.
- *`ENABLE` left out of `RASR`.* The output is that of the empty body:
  `CFSR=0x8`.
- *A size field for 4 KB at `0x20000000`.* Task A runs completely.
  After `starting task B`, `MemManage: CFSR=0x8`: task B's stack, at
  `0x20001000`, is outside the region.
- *Read-only permission.* `CFSR=0x82` at `address 0x20000400`: task A's
  first store, to `a_lines`, was refused.
- *The per-task function of A2 in place of this stage.* Everything runs,
  and `cross` stops at `0x20000400`. The attempt skips the stage: the
  limit that A1 exists to show is not seen. Run `cross` with a region
  over both partitions to see it.

**Solution.** One region over both partitions: 8 KB, which is 2^13, at
`0x20000000`.

```c
void mpu_set_task_region( uint32_t base, uint32_t size )
{
    ( void ) base;
    ( void ) size;
    MPU->RBAR = 0x20000000 | MPU_RBAR_VALID | 1;
    MPU->RASR = MPU_RASR_XN | MPU_RASR_AP( MPU_AP_FULL_RW )
              | MPU_RASR_S | MPU_RASR_C
              | MPU_RASR_SIZE( 13 ) | MPU_RASR_ENABLE;
    __asm volatile ( "dsb" ::: "memory" );
    __asm volatile ( "isb" ::: "memory" );
}
```

`make run` prints task A's three uptime lines, `task A exited`,
`starting task B`, and task B's uptime lines. `make run DEMO=rogue`
prints

```text
B: writing to the kernel's uptime counter
MemManage: CFSR=0x82
  address 0x20002004
  cause: data access
```

(`CFSR=0x82` is architectural: `DACCVIOL` and `MMARVALID`; the address
is this build's `uptime_ms`). `make run DEMO=cross` prints

```text
B: writing into task A's partition
B: read back 0xdeadbeef
B: uptime 4000 ms (second 1)
```

and continues. The store succeeded: both partitions are inside the one
region, whose permissions are the same for both. The kernel is
protected from the tasks, and the tasks are not protected from each
other.

At this stage the window is inside the region too: `DEMO=share` shows
task B reading `hello`, `sys_write returned -1` (the kernel's check
still describes B's own partition only), and then `B: window written`
with no fault.

### A2. The tasks against each other

`cross` succeeded because a region holds one set of permissions over
one range, and both partitions are in the range. Telling the tasks apart
takes one range per task, programmed when the task starts. The function
receives the partition, so the two fixed numbers become its arguments:

```c
void mpu_set_task_region( uint32_t base, uint32_t size )
{
    uint32_t log2_size = __builtin_ctz( size );
    MPU->RBAR = base | MPU_RBAR_VALID | 1;
    MPU->RASR = MPU_RASR_XN | MPU_RASR_AP( MPU_AP_FULL_RW )
              | MPU_RASR_S | MPU_RASR_C
              | MPU_RASR_SIZE( log2_size ) | MPU_RASR_ENABLE;
    __asm volatile ( "dsb" ::: "memory" );
    __asm volatile ( "isb" ::: "memory" );
}
```

`__builtin_ctz` counts the trailing zero bits of its argument, which for
a power of two is its logarithm: `__builtin_ctz( 0x1000 )` is 12. The
function is the checkpoint of week 4.

`make run DEMO=cross` now prints

```text
B: writing into task A's partition
MemManage: CFSR=0x82
  address 0x20000400
  cause: data access
```

`rogue` stops at `0x20002004` as before. `DEMO=share` stops at the first
read of the window, `MemManage: CFSR=0x82` at `address 0x20000f00`:
task B has no access to any part of A's partition.

What a task still cannot do is share anything: the isolation that stops
a store also stops a view that was wanted. A3 adds the view.

### A3. A shared window

Three changes, each with its own symptom when it is the one missing.

**First attempts.**

- *Region 2 granted, `sys_write_impl` unchanged.* `DEMO=share` prints
  `B: reading the window: hello`, so the hardware grants the read, and
  then `sys_write returned -1`: the kernel's check does not know the
  window. The store at the end faults at `0x20000f00` as it should.
- *Region 2 with `MPU_AP_FULL_RW`.* The store succeeds and
  `B: window written` is printed, with no fault: the permission field is
  what separates a view from a channel.

**Solution.** `mpu_set_task_region` programs region 2 after region 1.
The owner of the window, the task whose partition spans it, gets region
2 switched off, and the other tasks get the window read-only:

```c
    uint32_t w0 = ( uint32_t ) _shared_start;
    uint32_t w1 = ( uint32_t ) _shared_end;
    MPU->RBAR = w0 | MPU_RBAR_VALID | 2;
    if ( w0 >= base && w1 <= base + size )
    {
        MPU->RASR = 0;                      /* the owner: region 2 off */
    }
    else
    {
        MPU->RASR = MPU_RASR_XN | MPU_RASR_AP( MPU_AP_RO )
                  | MPU_RASR_S | MPU_RASR_C
                  | MPU_RASR_SIZE( 8 ) | MPU_RASR_ENABLE;   /* 2^8 = 256 B */
    }
```

with `extern char _shared_start[], _shared_end[];` at the top of the
file. In `kernel/syscall.c`, `sys_write_impl` accepts a buffer inside
the window for any task, as the system call only reads it:

```c
    if ( t != 0
         && !in_range( a0, len, t->part_start, t->part_end )
         && !in_range( a0, len, ( uint32_t ) _shared_start, ( uint32_t ) _shared_end )
         && !in_range( a0, len, 0x00000000, 0x00400000 ) )
        return ( uint32_t ) -1;
```

Read back in gdb at the end of `mpu_set_task_region` for task B
(`set var MPU->RNR = 2`, then `p/x MPU->RBAR` and `p/x MPU->RASR`),
region 2 holds `RBAR = 0x20000f02` (base `0x20000f00`, region 2) and
`RASR = 0x1606000f`: `XN` (bit 28), `AP = 6` (read-only), `S`, `C`,
`SIZE = 7` (2^8) and `ENABLE`. For task A the same region reads
`RASR = 0x0`.

`make run DEMO=share` prints

```text
B: reading the window: hello
hello
B: sys_write returned 5
B: writing the window
MemManage: CFSR=0x82
  address 0x20000f00
  cause: data access
```

`cross`, `rogue`, `badptr` and `overflow` give what they gave before
(`overflow`: `CFSR=0x92` at `0x20000fdc`, causes `data access` and
`stacking`, this build's address), and the normal program prints the
output of week 4.

---

## Part B

### B1. Three defective kernels

**`broken/unaligned-base/mpu.c`.** Symptom: task A runs, `starting task B`
is printed, then `MemManage: CFSR=0x8`, and task B never prints.
`CFSR=0x8` is the unstacking error of the starter's first run again, now
at task B's start. Stop at the last line of `mpu_set_task_region` for
the second call and read region 1 back (`set var MPU->RNR = 1`):
`RBAR = 0x20001001` and `RASR = 0x13060019`. The base is `0x20001000`;
the `SIZE` field is 12, which is a region of 2^13 = 8 KB. A base is a
multiple of its region's size, and `0x20001000` is not a multiple of
`0x2000`. QEMU says so when asked: run the image with `-d guest_errors`
and it prints `DRBAR[1]: 0x20001000 misaligned to DRSR region size,
mask = 0x1fff`, and leaves the region unprogrammed. The defect is the
`+ 1` on the logarithm. The architecture requires the base to be a
multiple of the size; this lab does not rely on what a processor does
with a base that is not, and the emulator's report is how the defect is
found here.

**`broken/xn-flash/mpu.c`.** Symptom: `scratch-os: kernel started`, then
`HardFault: HFSR=0x40000000`. No task starts. `HFSR` bit 30 is `FORCED`:
a fault escalated. Stop at `HardFault_Handler`: `p/x SCB->CFSR` gives
`0x1`, an instruction-fetch violation, and the backtrace shows
`HardFault_Handler`, then `MemManage_Handler`, then `mpu_init` at
`kernel/mpu.c:33`, called from `main`. The fault was raised in
`mpu_init`, before any task, with the MPU enabled and region 0 now
marked execute-never: FLASH holds the kernel as well as the tasks, so
the next instruction fetch was refused, and the handler for that
refusal, in FLASH, could not run either. `MPU->RASR` of region 0 reads
`0x1602002b` against `0x0602002b` in the solution: bit 28, `XN`.

**`broken/wrapping-check/syscall.c`.** Symptom: with `DEMO=wrap`, the
solution answers `B: sys_write returned -1`; here the call returns 63
and the output after the message is bytes that are not text. The demo
passes a buffer that starts 16 bytes before the end of task B's
partition, with the length `0xFFFFFFF0`. The broken `in_range` computes
`p + len <= end`: `0x20001ff0 + 0xFFFFFFF0` wraps to `0x20001fe0`, which
is below `end`, so the test passes. The solution's form, `len <= end -
p`, cannot wrap, because `p <= end` is tested first. The kernel then
copied bytes from a buffer that runs past the partition into the
kernel's own variables (the 63 is the room in the send buffer). The
check that exists to stop that was the check that let it through.

### B2. A third task

Add `src/task_c.c`:

```c
static uint32_t c_value = 7;

void task_c( void )
{
    printf( "C: hello from task C (%u)\n", c_value );
}
```

declare it in `src/tasks.h`; in `scripts/mps2_m3.ld` repeat task B's
three sections with `c` for `b`, after B's, and add the `ASSERT`; in
`kernel/task.c` add the externs and a table entry for C **between** A
and B (B never returns, so a task after it would never run). The normal
run prints C's line between `task A exited` and `starting task B`, and
the kernel's variables begin at `0x20003000`: `uptime_ms` is at
`0x20003004`.

In a copy, one region over all the partitions: three partitions are
12 KB, which is not a power of two, so the smallest region that covers
them is 16 KB, base `0x20000000`, size field 14:

```c
    MPU->RBAR = 0x20000000 | MPU_RBAR_VALID | 1;
    MPU->RASR = MPU_RASR_XN | MPU_RASR_AP( MPU_AP_FULL_RW )
              | MPU_RASR_S | MPU_RASR_C
              | MPU_RASR_SIZE( 14 ) | MPU_RASR_ENABLE;
```

`make run DEMO=rogue` then prints no MemManage report at all. The store
to `uptime_ms` succeeded, and task B continues with
`B: uptime 305420896 ms (second 1)` (the last digits vary from run to
run): the kernel's counter continues from the value the demo wrote,
`0x12345678` (305419896), not from the time.
With the per-task function of A2, `rogue` stops at `address
0x20003004`. One region protects the kernel from the tasks only while
the tasks' memory is a power of two at an aligned base; a region per
task does not depend on that.

### B3. Device registers

With the MPU enabled and no region over the register, task B's store to
`0x40028000` stops in `MemManage: CFSR=0x82` at `address 0x40028000`.
This holds at A1, A2 and A3 alike: an unprivileged access that no region
covers is refused, whatever the address is. The store succeeded in week
4's laboratory because the MPU was not in force there; the privilege of
the task was the same.

To grant it, add after region 2 in `mpu_set_task_region`:

```c
    MPU->RBAR = 0x40028000 | MPU_RBAR_VALID | 3;
    MPU->RASR = MPU_RASR_XN | MPU_RASR_AP( MPU_AP_FULL_RW ) | MPU_RASR_B
              | MPU_RASR_SIZE( 5 ) | MPU_RASR_ENABLE;
```

The smallest region is 32 bytes (2^5), and `0x40028000` is a multiple
of 32. A device is never executable, and it takes the bufferable
attribute (`MPU_RASR_B`) and not the cacheable one. Task B then prints
`B: LED=0x3`: the register reads back the value stored.

### B4. Data that must not run

`make run DEMO=exec` prints

```text
B: running code from the stack
MemManage: CFSR=0x1
  cause: instruction fetch
```

The two instructions are in a local array, so in task B's partition,
which is region 1, which has `MPU_RASR_XN`. The fetch of the first
instruction is refused: `CFSR=0x1` is `IACCVIOL` (architectural), and no
address is printed because `MMARVALID` is not set. With `MPU_RASR_XN`
removed from the description of region 1 in a copy, the array runs and
the output is `B: injected code returned 42`. Region 0 holds code, so it
cannot be execute-never; region 1 holds data, so it is. A program that
can be made to run its own data runs whatever is written there; the flag
closes that at no cost to a task that only reads and writes.

### B5. A pool inside a partition (optional)

One allocator that gives the outcome the exercise predicts: a first-fit
search over a pool of 1024 bytes, each block with an 8-byte header that
holds its size and whether it is in use, free blocks that follow one
another merged when a request is searched for.

```c
/* B5: a first-fit allocator over a pool inside this task's partition.
 * Each block starts with a header holding its size (header included)
 * and whether it is in use. */
#define POOL_SIZE 1024
#define HDR       8

struct hdr
{
    uint32_t size;
    uint32_t used;
};

static uint8_t pool[ POOL_SIZE ] __attribute__(( aligned( 8 ) ));
static int pool_ready;

static void *pool_alloc( uint32_t n )
{
    if ( !pool_ready )
    {
        struct hdr *first = ( struct hdr * ) pool;
        first->size = POOL_SIZE;
        first->used = 0;
        pool_ready = 1;
    }
    uint32_t need = ( ( n + 7 ) & ~7u ) + HDR;
    for ( uint32_t off = 0; off < POOL_SIZE; )
    {
        struct hdr *h = ( struct hdr * ) ( pool + off );
        if ( !h->used )
        {
            /* Merge the free blocks that follow into this one. */
            while ( off + h->size < POOL_SIZE
                    && !( ( struct hdr * ) ( pool + off + h->size ) )->used )
            {
                h->size += ( ( struct hdr * ) ( pool + off + h->size ) )->size;
            }
            if ( h->size >= need )
            {
                if ( h->size >= need + HDR + 8 )
                {
                    struct hdr *rest = ( struct hdr * ) ( pool + off + need );
                    rest->size = h->size - need;
                    rest->used = 0;
                    h->size = need;
                }
                h->used = 1;
                return pool + off + HDR;
            }
        }
        off += h->size;
    }
    return 0;
}

static void pool_free( void *p )
{
    ( ( struct hdr * ) ( ( uint8_t * ) p - HDR ) )->used = 0;
}
```

Seven requests of 120 bytes take 7 x 128 = 896 bytes with their headers.
Free the second, fourth and sixth blocks: 3 x 128 = 384 bytes, plus the
128 bytes at the end of the pool, 512 bytes in all, but in four pieces
of 128 bytes with a used block between each. A request for 256 bytes
needs 264 with its header, and no piece holds it: `pool_alloc( 256 )`
returns 0. Free the third block and the second, third and fourth are
three free blocks in a row, 384 bytes once merged, and the same request
succeeds. Free memory is not allocatable memory: the difference is
fragmentation, which a static partition does not have.

Run in task B after the greeting, with the allocator and a small
function that performs the sequence (blocks numbered from 0, so the
second, fourth and sixth are 1, 3 and 5), the output is

```text
B: 7 blocks of 120 bytes allocated
B: alloc(256) with blocks 1, 3 and 5 free: NULL
B: alloc(256) after block 2 is freed: ok
```
