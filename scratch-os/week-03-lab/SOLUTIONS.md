# Week 3 laboratory — worked solutions

Solutions to every exercise are provided here. You are expected to
attempt an exercise before consulting its solution: the values an
exercise asks you to record are the substance of the work, and reading a
recorded value does not substitute for taking one.

Task document: [`README.md`](README.md). Completed code:
[`solution/`](solution/).

Every value, diagnostic and listing below was produced by executing the
stated command using the course toolchain (Arm GNU Toolchain 15.3,
QEMU 11.1): against `solution/`, or, where an exercise asks for a value
at an intermediate stage, against `solution/` with the later additions
removed to recreate that stage. Each recorded value is also classified by
what fixes it:

- **architectural** — the ARMv7-M architecture defines it; any
  Cortex-M3 produces it;
- **linker script** — `scripts/mps2_m3.ld` fixes it, so your image
  produces the same value;
- **this build** — it depends on the code the compiler generated, so
  your value may differ if your code differs from `solution/`; the
  relation the exercise asks about holds regardless.

---

## Part A

### A1. Carry your Makefile across, and adapt it

**Step 1.** The `Makefile` from week 2's laboratory lists
`boot/startup.c`. Its derivation turns that into `build/boot/startup.o`,
and the only rule able to produce a `.o` requires the corresponding
`.c`. `boot/startup.c` does not exist in this tree, so `make` finds no
rule for the object it was asked for. The name it reports is not in the
`Makefile` at all: it is the product of the derivation.

**Step 2.** Correcting `SOURCES` alone builds an image that runs
correctly. The `print-%` rule from `lecture-notes/week-01.md` §6 shows
why:

```text
SOURCES = boot/startup.s src/main.c
OBJECTS = boot/startup.s build/src/main.o
```

`$(SOURCES:%.c=$(BUILD_DIR)/%.o)` rewrites only the words ending in
`.c`, and leaves `boot/startup.s` exactly as it was. That source path
therefore sits in `OBJECTS`, and the link command passes it to `gcc`
alongside the object:

```text
arm-none-eabi-gcc … boot/startup.s build/src/main.o -o build/scratch-os.axf
```

`gcc` recognises a `.s` file and assembles it as part of the link. The
image is correct, but no `build/boot/startup.o` is ever made, and the
`Makefile` does not express what it was written to express: each source
compiled separately into `build/`.

**Step 4.** Two lines in `solution/Makefile` correct the derivation,
the second mapping the `.s` words the first left unchanged:

```make
OBJECTS := $(SOURCES:%.c=$(BUILD_DIR)/%.o)
OBJECTS := $(OBJECTS:%.s=$(BUILD_DIR)/%.o)
```

and a second pattern rule assembles them:

```make
$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
```

`../week-02/solution/Makefile` writes the derivation instead as two
nested `patsubst` calls; either is correct. The build now runs:

```sh
cd ../week-03-lab/starter
make clean
make
```

and the link command names only objects:

```text
arm-none-eabi-gcc … build/boot/startup.o build/src/main.o -o build/scratch-os.axf
```

### A2. Add a debug target and a hook for emulator options

`solution/Makefile`:

```make
QEMUFLAGS := -machine mps2-an385 -monitor null -semihosting \
             --semihosting-config enable=on,target=native \
             -kernel $(BIN) -serial stdio -nographic
QEMU_EXTRA :=

run: $(BIN)
	qemu-system-arm $(QEMUFLAGS) $(QEMU_EXTRA)

debug: $(BIN)
	qemu-system-arm $(QEMUFLAGS) -S -gdb tcp::1234 $(QEMU_EXTRA)
```

with `debug` added to `.PHONY`. Attached at reset:

```text
(gdb) info registers sp pc
sp             0x20400000          0x20400000
pc             0x40                0x40 <Reset_Handler>
(gdb) x/2wx 0
0x0 <isr_vector>:	0x20400000	0x00000041
```

| Value | Fixed by |
|--|--|
| `sp` = `0x20400000`, vector word 0 | linker script (`_estack`, the top of RAM); loaded into `sp` from word 0 by the processor, which is architectural |
| `pc` = `0x40`, vector word 1 = `0x41` | linker script and the size of the table: `.isr_vector` is 16 words, 64 bytes, and `Reset_Handler` is placed immediately after it; bit 0 set in the stored word is architectural (the Thumb bit) |

Week 2's base image held a two-word table, eight bytes, so its
`Reset_Handler` began at `0x8`. The table now holds sixteen words, so
the code begins at `16 × 4 = 64 = 0x40`.

**The two recipe faults.** At this stage `main` still prints its two
lines and returns. Without `-S`, QEMU runs the program at once;
the two lines are printed before gdb is started, and on attaching gdb
reports:

```text
pc             0x9e                0x9e <Reset_Handler+94>
```

which is the `b hang` loop `Reset_Handler` enters when `main` returns.
Setting `QEMUFLAGS` on the command line replaces the list that named the
machine:

<!-- verify: expect-failure (fixture a2-qemuflags-override asserts this) -->
```sh
make run QEMUFLAGS="-d unimp,guest_errors"
```

```text
qemu-system-arm: No machine specified, and there is no default
```

### A3. Write a fault handler, and provoke its fault

**Step 1.** With the division at the start of `main` and no handler,
nothing is printed and the program does not end.

**Step 2.** Interrupted with `Ctrl-C` in gdb:

```text
Program received signal SIGINT, Interrupt.
Default_Handler () at boot/startup.s:406
(gdb) info registers pc xpsr
pc             0xcc                0xcc <Default_Handler>
xpsr           0x61000006          1627389958
(gdb) p $xpsr & 0x1ff
$1 = 6
```

Exception 6 is **UsageFault** (architectural). The program is silent
not because it stopped but because it is executing `Default_Handler`'s
single instruction, `b Default_Handler`, indefinitely.

**Step 3.**

```c
void UsageFault_Handler( void )
{
    semihost_write0( "UsageFault_Handler: a usage fault happened\n" );
    for ( ;; )
    {
    }
}
```

prints `UsageFault_Handler: a usage fault happened`.

**Step 4.** The stacked frame, read at the handler's first instruction
with `DEMO_DIVIDE_BY_ZERO` set to 1:

```text
(gdb) x/8wx $sp
0x203fffc8:	0xe000ed18	0x000000e0	0x0000002a	0x00000000
0x203fffd8:	0x00000000	0x0000009f	0x0000015e	0x61000000
(gdb) x/i *(unsigned *)($sp + 24)
   0x15e <main+18>:	udiv	r3, r2, r3
```

| Word | Register | Value | Fixed by |
|--|--|--|--|
| 0 | `r0` | `0xe000ed18` | this build: left over from `Reset_Handler`'s priority setup |
| 1 | `r1` | `0x000000e0` | this build: likewise |
| 2 | `r2` | `0x0000002a` | this build: the dividend, 42 |
| 3 | `r3` | `0x00000000` | this build: the divisor, 0 |
| 4 | `r12` | `0x00000000` | this build |
| 5 | `lr` | `0x0000009f` | this build: `main`'s return address into `Reset_Handler`, Thumb bit set |
| 6 | return address | `0x0000015e` | this build for the value; **architectural** that it designates the faulting instruction |
| 7 | `xPSR` | `0x61000000` | architectural bit 24 (Thumb); the condition flags are this build's |

The order of the eight words is architectural. The stacked return
address designates the `udiv` itself, so that the fault's cause can be
located; the instruction is `udiv` rather than `sdiv` because the
operands are `uint32_t`. Its operands are `r2` (42) and `r3` (0).

**Step 5.** At the same stop:

```text
(gdb) p/x $lr
$1 = 0xfffffff9
(gdb) x/2wx 0xE000ED28
0xe000ed28:	0x02000000	0x00000000
(gdb) p/x $sp
$2 = 0x203fffc8
(gdb) break UsageFault_Handler
(gdb) continue
(gdb) p/x $sp
$3 = 0x203fffc0
```

`lr` is `0xfffffff9`, the exception-return value: no instruction lies at
that address, and the processor recognises the value when the handler
returns. It differs from the stacked `lr`, `0x9f`, which belongs to the
interrupted code (architectural). CFSR is `0x02000000`: bit 25, which is
`DIVBYZERO` (architectural). HFSR is `0` because a HardFault was not
taken; the UsageFault was taken as itself.

The stack pointer is `0x203fffc8` at the first instruction and
`0x203fffc0` after the prologue, 8 bytes lower: the compiler's
`push {r7, lr}` (this build). A routine that must find the frame by
reading `sp` would read the moved value; the amount depends on the
compiler and its options, so such a routine must read `sp` before any
compiler-generated code runs.

The other three handlers take the same form, each printing its own name;
see `solution/src/main.c`.

### A4. Drop privilege, and return through a system call

**Step 1.** Without `SVC_Handler`, `main`'s two lines are printed and
then nothing; interrupted in gdb, `pc` is `0xcc <Default_Handler>` and
`$xpsr & 0x1ff` is `11`, **SVCall** (architectural).

**Step 2.**

```c
void SVC_Handler( void )
{
    semihost_write0( "SVC_Handler: privileged code printing on behalf of unprivileged code\n" );
}
```

**Step 3.**

```text
(gdb) p/x $control
$1 = 0x0
(gdb) break *SVC_Handler
(gdb) continue
(gdb) p/x $control
$2 = 0x1
(gdb) p $xpsr & 0x1ff
$3 = 11
(gdb) p/x $lr
$4 = 0xfffffff9
(gdb) x/8wx $sp
0x203fffd0:	0x00000001	0x20000000	0x20000020	0x00000000
0x203fffe0:	0x00000000	0x0000015f	0x00000170	0x01000000
(gdb) x/i *(unsigned *)($sp + 24) - 2
   0x16e <main+34>:	svc	0
```

CONTROL is `0x0` in `main` and `0x1` in the handler: nPRIV remains set
(architectural). `lr` is `0xfffffff9` here as in A3: the value does not
depend on which exception was taken, only on the state it interrupted
(architectural, for an interrupted Thread mode using the main stack). The handler is nonetheless privileged, because it
executes in Handler mode, which is always privileged whatever CONTROL
holds.

The stacked return address is `0x170` (this build), two bytes after the
`svc 0` at `0x16e`. A fault stacks the address of the instruction that
faulted, since that instruction did not complete and the cause must be
locatable. A supervisor call is a request that **has** completed, and
when its handler returns execution must continue with the next
instruction rather than issue the request again; the processor
therefore stacks the address that follows it (architectural).

### A5. Touch SysTick without privilege

```c
volatile uint32_t *systick_ctrl = ( volatile uint32_t * ) 0xE000E010;
*systick_ctrl = 0;
```

```text
main: privileged hello
main: this text lives in .data
SVC_Handler: privileged code printing on behalf of unprivileged code
BusFault_Handler: a bus fault happened
```

```text
(gdb) break *BusFault_Handler
(gdb) continue
(gdb) p $xpsr & 0x1ff
$1 = 5
(gdb) x/8wx $sp
0x203fffd0:	0x00000001	0x20000000	0x00000000	0xe000e010
0x203fffe0:	0x00000000	0x0000015f	0x00000178	0x41000000
(gdb) x/i *(unsigned *)($sp + 24)
   0x178 <main+44>:	str	r2, [r3, #0]
```

Exception 5 is **BusFault**. The stacked return address designates the
store, and `r3`, the base register of that store, holds `0xe000e010`,
the address it attempted. ARMv7-M permits a bus fault on a write to be
reported imprecisely, after later instructions have executed, in which
case the stacked address would follow the store instead; QEMU reports
this one precisely.

---

## Part B

### B1. Separate the platform from the program

With the solution unchanged, `make run QEMU_EXTRA="-d unimp,guest_errors"`
prints the four lines of A5 and **no diagnostic line**. The BusFault is
not a platform limitation: the processor refused an unprivileged access
to the System Control Space, which is what the architecture requires.
QEMU implements SysTick, so there is nothing for the diagnostic to
report.

With the address changed to `0x40010000`:

```text
main: privileged hello
main: this text lives in .data
SVC_Handler: privileged code printing on behalf of unprivileged code
cmsdk-ahb-gpio: unimplemented device write (size 4, offset 0x000, value 0x00000000)
```

`BusFault_Handler` does not run, and on attaching gdb finds `pc` in
`Reset_Handler`'s `b hang`: `main` returned normally. Unprivileged code
may access the peripheral region at `0x40000000`, which the table in
"What unprivileged code cannot do" does not restrict, so the store is
permitted. It reached GPIO0, which QEMU instantiates as a stub, and the
value was discarded.

| Run | Responsible | The observation that told you |
|--|--|--|
| `0xE000E010` | the program: an unprivileged write to the System Control Space | `BusFault_Handler` ran, and no diagnostic line appeared |
| `0x40010000` | the platform: the device is not implemented | a `cmsdk-ahb-gpio: unimplemented device write` line, and no fault |

### B2. Read the core vector table out of the image

```sh
cd ../week-03-lab/starter
make
arm-none-eabi-size -A build/scratch-os.axf
arm-none-eabi-objdump -s -j .isr_vector build/scratch-os.axf
arm-none-eabi-nm -n build/scratch-os.axf
```

`.isr_vector` is 64 bytes, **16 words**: exactly entries 0 to 15 of the
exceptions table, the initial stack pointer and the fifteen system
exceptions. Entries 16 and above, the device interrupts, are absent.

```text
 0000 00004020 41000000 cd000000 e9000000  ..@ A...........
 0010 fd000000 11010000 25010000 00000000  ........%.......
 0020 00000000 00000000 00000000 39010000  ............9...
 0030 cd000000 00000000 cd000000 cd000000  ................
```

| Slot | Word | Symbol |
|--|--|--|
| 0 | `0x20400000` | `_estack` |
| 1 | `0x00000041` | `Reset_Handler` + 1 |
| 2 | `0x000000cd` | `Default_Handler` + 1 (NMI) |
| 3 | `0x000000e9` | `HardFault_Handler` + 1 |
| 4 | `0x000000fd` | `MemManage_Handler` + 1 |
| 5 | `0x00000111` | `BusFault_Handler` + 1 |
| 6 | `0x00000125` | `UsageFault_Handler` + 1 |
| 7–10 | `0` | reserved |
| 11 | `0x00000139` | `SVC_Handler` + 1 |
| 12 | `0x000000cd` | `Default_Handler` + 1 (DebugMonitor) |
| 13 | `0` | reserved |
| 14, 15 | `0x000000cd` | `Default_Handler` + 1 (PendSV, SysTick) |

The addresses are this build's. In the starter, before any handler was
written, slots 3 to 6 and 11 held `0x000000cd` as well.

```text
000000cc W DebugMon_Handler
000000cc T Default_Handler
000000cc W NMI_Handler
000000cc W PendSV_Handler
000000cc W SysTick_Handler
000000e8 T HardFault_Handler
000000fc T MemManage_Handler
00000110 T BusFault_Handler
00000124 T UsageFault_Handler
00000138 T SVC_Handler
```

`W` marks a weak definition and `T` a strong one in `.text`. The four
names still `W` are the weak aliases `boot/startup.s` declares, all at
`Default_Handler`'s address, because nothing else defines them; the five
you wrote are `T`, and their slots are the five that changed.

### B3. Which service was requested

```text
(gdb) x/hx *(unsigned *)($sp + 24) - 2
0x16e <main+34>:	0xdf00
```

With `svc #7`:

```text
0x16e <main+34>:	0xdf07
```

The upper byte, `0xdf`, identifies the instruction; the lower byte is
the number (architectural: the Thumb `svc` encoding carries an 8-bit
immediate). To obtain it for itself, `SVC_Handler` would take the stack
pointer at entry, read the stacked return address at offset 24,
subtract 2 to reach the `svc` instruction, read the halfword there, and
keep its low byte. A handler written in C must do this before its own
prologue changes the stack pointer, which is why such handlers usually
begin with a few instructions of assembly.

### B4. Follow a call, a watched value and a write

**Part 1.**

```text
(gdb) bt
#0  semihost_write0 (message=0x290 "main: privileged hello\n") at src/main.c:49
#1  0x00000158 in main () at src/main.c:160
(gdb) info registers r0 lr
r0             0x290               656
lr             0x159               345
(gdb) x/s $r0
0x290:	"main: privileged hello\n"
(gdb) info symbol $lr
main + 13 in section .text
(gdb) finish
main () at src/main.c:165
165	    semihost_write0( data_message );
```

`r0` holds the address of the string, the first argument; `lr` holds the
return address (architectural). `lr` is `0x159`, one more than the
`0x158` that `bt` gives for the caller's frame: bit 0 marks Thumb code,
and the instruction is at `0x158`. The symbol is `main + 13`, the
instruction after the call; the addresses are this build's. `finish`
stops at line 165, the statement after the call; the function returns no
value, so `r0` afterwards is not a result.

**Part 2.**

```text
(gdb) tbreak semihost_write0
Temporary breakpoint 2 at 0xd8: file src/main.c, line 49.
(gdb) continue
Temporary breakpoint 2, semihost_write0 (message=0x20000000 <data_message> "main: this text lives in .data\n") at src/main.c:49
(gdb) info registers r0
r0             0x20000000          536870912
(gdb) info breakpoints
No breakpoints, watchpoints, tracepoints, or catchpoints.
```

`r0` is `0x20000000`, against `0x290` in Part 1. A string literal is
read-only data kept in flash, which begins at 0 (architectural: code
region); `data_message` is an initialised variable, so it lives in RAM at
`0x20000000` (architectural: SRAM region), where `Reset_Handler`'s
`.data` copy placed its initial value (this build for the addresses). The
temporary breakpoint removed itself when hit, so `info breakpoints`
lists none.

**Part 3.**

```text
Hardware watchpoint 2: systick_ctrl
Hardware watchpoint 2: systick_ctrl
Old value = (volatile uint32_t *) 0x0 <isr_vector>
New value = (volatile uint32_t *) 0xe000e010
main () at src/main.c:213
213	    *systick_ctrl = 0;
```

The initialisation `systick_ctrl = ( volatile uint32_t * ) 0xE000E010`
changed the variable; a watchpoint reports after the store completes, so
line 213, the write through the pointer that faults, is the statement
about to execute. The old value is this build's: the stack slot held 0.

**Part 4.**

```text
0x40010010:	0x00000000
0x40028000:	0x00000003
0x40080000:	0x00000000
```

and QEMU's terminal prints:

```text
cmsdk-ahb-gpio: unimplemented device write (size 4, offset 0x010, value 0x00000001)
cmsdk-ahb-gpio: unimplemented device read  (size 4, offset 0x010)
RESERVED 4: unimplemented device write (size 4, offset 0x050000, value 0xdeadbeef)
RESERVED 4: unimplemented device read  (size 4, offset 0x050000)
```

`0x40010000`, GPIO0, is a **placeholder**: the write was logged and
dropped, and the read returned 0. `0x40028000`, the board's LED register,
is **modelled**: it kept `3`, and QEMU printed nothing. `0x40080000` lies
in no device's range: QEMU names the region, `RESERVED 4`, and again
returns 0. None of the three faulted. The line naming the device, the
line naming a region, and the absence of any line are the three
observations that told them apart.
