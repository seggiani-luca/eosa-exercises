# Week 3 laboratory — exceptions, privilege, and the debugger

This laboratory covers week 2's topic, boot and CPU configuration, and
extends the work done in class. Its subject is the **exception side** of
week 2: the fault handlers, the system call, and the transition to
unprivileged execution. The boot side — the vector table, the `.data`
copy, the `.bss` clear and the CPU configuration — was written in class
and is supplied complete. The exception mechanism and the debugger
practice developed here are what week 3's topic, devices, interrupts and
system calls, builds on; the closing section identifies where. You work
in [`starter/`](starter/), derived from `week-02/solution/`; worked
solutions are in [`SOLUTIONS.md`](SOLUTIONS.md).

## Objectives

On completion you will have:

- adapted the `Makefile` you constructed in week 2's laboratory to an
  image whose boot code is written in assembly, and established by
  inspection, rather than by the build succeeding, that every source
  is compiled to an object;
- extended that `Makefile` with a target that holds the processor at
  reset for a debugger, and with a means of passing further emulator
  options without editing a recipe;
- identified, with the debugger, which exception a silently looping
  program is handling, from the exception number the processor records
  in xPSR;
- written fault handlers and a supervisor-call handler, and accounted
  for the vector table selecting them without being edited, in terms of
  weak and strong symbol definitions;
- recorded the CONTROL register before and after the drop to
  unprivileged execution, and the eight-word frame the processor stacks
  on exception entry, and distinguished the return address a supervisor
  call stacks from the one a fault stacks;
- distinguished, with the emulator's diagnostic output, an access the
  architecture refuses from an access to a device the emulator does not
  implement.

## Starting point

[`starter/`](starter/), a prepared copy of
[`../week-02/solution/`](../week-02/solution/). Its `boot/startup.s` and
`scripts/mps2_m3.ld` are complete and unmodified. Its `src/main.c` has
had the exception side removed: the four fault handlers, `SVC_Handler`,
the divide-by-zero switch, the drop to unprivileged execution, the
`svc #0`, and the write to SysTick's control register. It has **no
`Makefile`**: exercise A1 has you bring the one you constructed in
[`../week-02-lab/`](../week-02-lab/).

## Solutions

Worked solutions to every exercise are provided in
[`SOLUTIONS.md`](SOLUTIONS.md), and the completed code in
[`solution/`](solution/). You are expected to attempt each exercise
before consulting its solution: the values an exercise asks you to
record are the substance of the work, and reading a recorded value does
not substitute for taking one.

## Structure

The exercises are divided into two parts. **Part A** is intended for
completion during the laboratory session, where an assistant is
available. **Part B** is designed to be carried out independently
afterwards; it requires no resource beyond the repositories, the
published solutions, and the standard toolchain.

Part A uses the debugger throughout. The reference for every command is
`lecture-notes/week-02.md`, in the subsections "A remote debugger: two
programs, two terminals", "The gdb commands you need" and "Attaching at
reset". The debugger is the one installed by following
`docs/environment-setup.md` in `course-material`, which gives its name
for each supported environment; this document calls it
`arm-none-eabi-gdb`.

---

## Part A — in the laboratory

### A1. Carry your Makefile across, and adapt it

Copy the `Makefile` you constructed in week 2's laboratory into
`starter/`. If you did not complete it, copy
`../week-02-lab/solution/Makefile` instead; the exercise is the same.

1. Run `make`. It stops at once with
   `No rule to make target 'build/boot/startup.o'` (older versions of
   GNU Make quote it as `` `build/boot/startup.o' ``).
   Account for the message: which file does your `SOURCES` name, does it
   exist in this tree, and from where does `make` obtain the name it
   reports?
2. Correct `SOURCES` to name `boot/startup.s`, and nothing else. Run
   `make`, then `make run`. The image builds and prints its two lines.
3. **The build succeeding does not show that it is correct.** List
   `build/`. There is no `build/boot/` and no `startup.o`. Determine why
   by asking the `Makefile` what `OBJECTS` expanded to, as
   `lecture-notes/week-01.md` §6, "Making a Makefile report its own
   state", describes, and by reading the link command `make` printed.
   A substitution reference rewrites only the words it matches and
   leaves every other word unchanged; account for where
   `boot/startup.s` ended up and which program consequently assembled
   it.
4. Correct the derivation so that both `.c` and `.s` sources map to
   objects under `build/`, and add a pattern rule for `.s` sources
   alongside the one for `.c`. `gcc` passes a `.s` file to the
   assembler, and passing it `$(CFLAGS)` keeps `-mthumb -mcpu=cortex-m3`
   in step with the C objects. `../week-02/solution/Makefile` contains
   one way to write both.

Then, from a clean tree:

```sh
cd ../week-03-lab/starter
make clean
make
make run
```

`make run` prints:

```text
main: privileged hello
main: this text lives in .data
```

Stop QEMU with `Ctrl-C`. Two lines are all the starter produces: the
first comes from a string literal in flash, the second from a variable
the `.data` copy loop placed in RAM, and `main` then returns. Week 2's
finished program prints four; the other two come from the code
exercises A3 to A5 restore.

**Checkpoint:** `build/boot/startup.o` exists, the link command names
only objects, and you can state why step 2 built and ran regardless.

### A2. Add a debug target and a hook for emulator options

`lecture-notes/week-02.md`, "A remote debugger: two programs, two
terminals", describes week 2's `debug` target as the `run` command plus
two options, `-S` and `-gdb tcp::1234`, and states that you can
reconstruct it for any image. Your `Makefile` has no such target. Add
it, together with a second addition the later exercises rely on.

1. Separate the emulator options in your `run` recipe into a variable,
   for instance `QEMUFLAGS`, so that `run` and `debug` share them.
2. Add a second, empty variable, for instance `QEMU_EXTRA`, appended to
   both invocations. It exists to be set on the command line:
   `make run QEMU_EXTRA="…"` then **adds** an option to the invocation.
   Part B uses it. Setting `QEMUFLAGS` on the command line instead would
   **replace** the whole list.
3. Add the phony target `debug`: the `run` invocation with `-S` and
   `-gdb tcp::1234` added.

Test it with two terminals. In the first:

```text
make debug
```

Nothing is printed: the processor is held at reset. In the second, from
the same directory:

```text
arm-none-eabi-gdb build/scratch-os.axf
(gdb) target remote :1234
(gdb) info registers sp pc
(gdb) x/2wx 0
```

**Record** `sp`, `pc`, and the two words at address 0. Week 2's notes,
in "Attaching at reset", show `pc` equal to `0x8` for the base image,
whose table held two words. Account for the value you record instead,
from the size of the table `boot/startup.s` now builds. Then
`continue`: the two lines appear in the first terminal. Leave gdb with
`quit` and stop QEMU with `Ctrl-C`.

Two faults in the recipe present as debugger problems; both are faults
of the recipe.

- **`-S` omitted.** The two lines appear in the first terminal at once,
  before gdb is started, and when gdb attaches the program has already
  finished: `pc` is in `hang`, the loop at the end of `Reset_Handler`.
- **`QEMUFLAGS` set on the command line rather than `QEMU_EXTRA`.**
  QEMU stops immediately, reporting
  `No machine specified, and there is no default`: the list that named
  the machine has been replaced, not extended.

**Checkpoint:** `make debug` holds the processor; `sp`, `pc` and the
two vector words recorded, and the value of `pc` accounted for.

### A3. Write a fault handler, and provoke its fault

Make `main` divide by zero. The reference is
`lecture-notes/week-02.md`, "Catching a divide by zero": add the switch
`#define DEMO_DIVIDE_BY_ZERO 1` after `#include <stdint.h>`, and at the
very start of `main` the `#if DEMO_DIVIDE_BY_ZERO` block that section
gives. Write no handler yet.

1. `make run`. Nothing is printed, not even the first of `main`'s two
   lines, since the division precedes them, and the program does not
   end. Stop QEMU.
2. Find out what the processor is doing. Run `make debug` and attach
   gdb as in A2, then `continue`, wait a moment, and interrupt with
   `Ctrl-C` in the gdb terminal. Then:

   ```text
   (gdb) info registers pc xpsr
   (gdb) p $xpsr & 0x1ff
   ```

   **Record** the function `pc` lies in and the exception number. The
   low nine bits of xPSR hold the number of the exception being handled
   (`lecture-notes/week-02.md`, "The Cortex-M3 registers"); look it up
   in the table in "Exceptions and faults: one family". The program is
   silent because the exception reached `Default_Handler`, which loops
   without printing.
3. Write `UsageFault_Handler`, in the form "Catching a divide by zero"
   gives: it prints its own name and loops. `make run` now prints
   `UsageFault_Handler: a usage fault happened`.

**Why the table now reaches your function without being edited.**
`boot/startup.s` declares every handler name **weak** and aliases it to
`Default_Handler` with `.thumb_set` (`lecture-notes/week-02.md`,
"Completing the vector table"). The table's `.word UsageFault_Handler`
is resolved by the linker. Before you wrote the function, the only
definition of that name was the weak alias, so the word held
`Default_Handler`'s address. Your function is a **strong** definition of
the same name, and the linker prefers a strong definition to a weak one,
so the same word now holds your function's address. Part B's B2 lets
you read this change out of the image.

4. **Record what the processor saved.** On exception entry the processor
   pushes eight words onto the stack, in this order from the lowest
   address: `r0`, `r1`, `r2`, `r3`, `r12`, `lr`, the return address, and
   `xPSR`. The return address is the seventh word, at `sp + 24`. Run
   `make debug`, attach, and stop at the handler's first instruction —
   before the compiler's prologue moves `sp` — by giving its address
   with `*`:

   ```text
   (gdb) break *UsageFault_Handler
   (gdb) continue
   (gdb) x/8wx $sp
   (gdb) x/i *(unsigned *)($sp + 24)
   ```

   **Record** the eight words, and the instruction the stacked return
   address designates. Identify, among `r0`–`r3`, the two operands of
   the division.

5. **Record three further values**, still stopped at the handler's first
   instruction. Each takes one command.

   ```text
   (gdb) p/x $lr
   (gdb) x/2wx 0xE000ED28
   (gdb) p/x $sp
   (gdb) break UsageFault_Handler
   (gdb) continue
   (gdb) p/x $sp
   ```

   - `lr` is not the stacked `lr` of step 4. On exception entry the
     processor writes into the register a value that is the address of
     no instruction; it recognises that value on return. Both are
     described in `lecture-notes/week-03.md`, "Exception entry: what the
     hardware saves". **Record** the value.
   - `0xE000ED28` and the word after it are the Configurable Fault
     Status Register (CFSR) and the HardFault Status Register (HFSR).
     The upper half of CFSR reports UsageFault causes: bit 16
     `UNDEFINSTR`, 17 `INVSTATE`, 18 `INVPC`, 19 `NOCP`, 24 `UNALIGNED`,
     25 `DIVBYZERO` (Armv7-M Architecture Reference Manual, the section
     on the Configurable Fault Status Register). **Record** both words, name
     the bit set in CFSR and the condition it reports, and state why
     HFSR holds no bit.
   - The two `p/x $sp` commands read the stack pointer at the handler's
     first instruction and after the compiler's prologue, the plain
     `break` having stopped there. **Record** both values and their
     difference, and state why a routine that must locate the stacked
     frame by reading `sp` cannot begin as a function whose prologue the
     compiler writes.

Finally write the other three fault handlers in the same form:
`HardFault_Handler`, `MemManage_Handler` and `BusFault_Handler`. Set
`DEMO_DIVIDE_BY_ZERO` back to `0`.

**Checkpoint:** the exception number recorded while the program was
silent; the handler's message printed; the stacked frame recorded, with
the faulting instruction and its operands identified; `lr`, the fault
status registers and the two stack-pointer values recorded and
accounted for.

### A4. Drop privilege, and return through a system call

The reference is `lecture-notes/week-02.md`, from "The CONTROL register"
through "Adding the system call".

1. After the two `semihost_write0` calls in `main`, add the block that
   sets CONTROL's nPRIV bit ("Dropping privilege: a one-way street"),
   followed by `__asm__ volatile ( "svc #0" : : : "memory" );`. Write no
   handler yet, and run. The two lines print, and then nothing: the
   `svc` reached `Default_Handler`. Confirm the exception number with
   gdb as in A3, step 2.
2. Write `SVC_Handler`. Unlike a fault handler it does not loop: it
   prints `SVC_Handler: privileged code printing on behalf of
   unprivileged code` and returns.
3. **Record CONTROL on both sides of the drop.** With `make debug` and
   gdb attached:

   ```text
   (gdb) break main
   (gdb) continue
   (gdb) p/x $control
   (gdb) break *SVC_Handler
   (gdb) continue
   (gdb) p/x $control
   (gdb) p $xpsr & 0x1ff
   (gdb) p/x $lr
   (gdb) x/8wx $sp
   (gdb) x/i *(unsigned *)($sp + 24) - 2
   ```

   **Record** CONTROL in `main` and in the handler, the exception
   number, `lr` (compare it with A3's), the stacked return address, and
   the instruction two bytes before it. The handler runs privileged although nPRIV is still set:
   account for that from "Touching the hardware without privilege".

Compare the stacked return address with A3's. There it designated the
instruction that faulted; here it designates the instruction **after**
the `svc`. Account for the difference in terms of what each exception
is for, and of where execution must resume when its handler returns.

**Checkpoint:** CONTROL recorded on both sides of the drop; `lr` in the
handler recorded; the supervisor call's stacked return address shown to
follow the `svc`.

### A5. Touch SysTick without privilege

After the `svc #0`, add the write to SysTick's control register given in
"Touching the hardware without privilege":

```c
volatile uint32_t *systick_ctrl = ( volatile uint32_t * ) 0xE000E010;
*systick_ctrl = 0;
```

`make run` now prints week 2's four lines:

```text
main: privileged hello
main: this text lives in .data
SVC_Handler: privileged code printing on behalf of unprivileged code
BusFault_Handler: a bus fault happened
```

With `break *BusFault_Handler`, repeat A3's step 4. **Record** the
exception number, the stacked return address and the instruction it
designates, and identify the stacked register that holds `0xe000e010`.

**Checkpoint:** four lines printed; the BusFault's stacked frame
recorded, with the faulting store and its target address identified.

### Done looks like — Part A

- `starter/` builds from your own `Makefile`, which compiles the
  assembly boot file to an object, and you can state why a partial
  adaptation built and ran regardless.
- `make debug` holds the processor at reset, and `QEMU_EXTRA` adds an
  option to an invocation without replacing it.
- The exception number of a silent program recorded from xPSR, and
  identified in the exception table.
- Four fault handlers and `SVC_Handler` written, and the selection of
  each by the vector table accounted for by weak and strong definitions.
- CONTROL recorded on both sides of the privilege drop.
- `lr` on exception entry, the fault status registers and the stack
  pointer before and after a prologue recorded, each accounted for.
- Three stacked frames recorded — UsageFault, SVCall and BusFault — and
  the difference between the return address a fault stacks and the one
  a supervisor call stacks accounted for.

Part A is self-contained. Part B requires none of the laboratory's
facilities.

---

## Part B — independent work

### B1. Separate the platform from the program

`lecture-notes/week-03.md`, "Touching a device that is not there", shows
that an access to a device QEMU does not implement neither faults nor
reports anything by default, and that `-d unimp` reveals it; the section
adds that `-d guest_errors` covers the remaining case, a modelled device
used wrongly. Use the hook from A2 to run with both options:

```text
make run QEMU_EXTRA="-d unimp,guest_errors"
```

1. Record whether any diagnostic line appears. The program ends in a
   BusFault: account for QEMU reporting nothing about it, in terms of
   what the diagnostic reports and what the fault is.
2. Change the address in A5's write from `0xE000E010` to `0x40010000`,
   GPIO0's data register, and run again with the same option. Record
   what is printed, whether `BusFault_Handler` runs, and where gdb finds
   the program afterwards.
3. Restore `0xE000E010`. State, for each of the two runs, whether the
   program or the platform was responsible for what you observed, and
   which observation told you. The table in "What unprivileged code
   cannot do" in `lecture-notes/week-02.md` names the region the first
   write fell in.

**Checkpoint:** two runs recorded, one faulting with no diagnostic and
one reporting a diagnostic with no fault, each accounted for.

### B2. Read the core vector table out of the image

Week 2's laboratory, B6, decoded the first two words of the table and
found one value recurring through the rest. Here you read the whole
table, and observe what your handlers changed in it.

```sh
cd ../week-03-lab/starter
make
arm-none-eabi-size -A build/scratch-os.axf
arm-none-eabi-objdump -s -j .isr_vector build/scratch-os.axf
arm-none-eabi-nm -n build/scratch-os.axf
```

1. From the `size` output, record the size of `.isr_vector` and convert
   it to a number of words. Compare it with the exceptions table in
   `lecture-notes/week-02.md`, "Exceptions and faults: one family":
   which entries does the table contain, and which entries of that
   table does it not?
2. Decode each word of the table (little-endian, as in week 2's B6) and
   name the symbol it designates, using `nm`. Identify which slots now
   hold your handlers and which still hold `Default_Handler`.
3. In the `nm` output, the handlers you wrote carry the type `T`. Which
   handler names still carry `W`, and at which address? Relate the two
   letters to the weak and strong definitions of A3.

**Checkpoint:** the table's size in words recorded and accounted for;
every slot decoded and attributed; the `W`/`T` distinction related to
the slots that changed.

### B3. Which service was requested

A single `svc` instruction is how unprivileged code requests any
privileged service, so the handler needs a way to tell requests apart.
The instruction carries one: `svc #0` encodes the number in its low
byte.

1. In A4 you recorded the instruction two bytes before the stacked
   return address. Repeat that observation with `x/hx` in place of
   `x/i`, and record the halfword.
2. Change `svc #0` to `svc #7`, rebuild, and repeat. Record the new
   halfword, and identify which bits changed.
3. State the steps `SVC_Handler` would perform, in terms of the stacked
   frame, to obtain the requested number for itself. Writing it is not
   required.

> Reference: `lecture-notes/week-02.md`, "This is a system call", for
> the role the `svc` instruction plays.

**Checkpoint:** two halfwords recorded and the number located in them;
the handler's steps stated.

### B4. Follow a call, a watched value and a write

The debugger commands below are those that a program with several
functions, a changing variable and a device needs; none appeared in
week 2's list. Each has one purpose.

| Command | Effect |
|--|--|
| `backtrace` (`bt`) | list the chain of calls that led to the current point, innermost first |
| `info symbol ADDR` | name the function or variable containing an address, with the offset |
| `finish` | run until the current function returns, and stop in its caller |
| `tbreak LOCATION` | a breakpoint that deletes itself the first time it is hit |
| `info breakpoints` | list the breakpoints, and how often each was hit |
| `watch EXPR` | stop when the value of an expression changes, reporting the old and the new value |
| `delete` | remove every breakpoint and watchpoint |
| `set {unsigned int}ADDR = V` | store the 32-bit value `V` at `ADDR`, as an access made by the debugger |

Work from your completed `starter/`, or from `solution/`, with the
`debug` target of A2.

**Part 1: a call.** `make debug` in one terminal; in the other:

```text
(gdb) target remote :1234
(gdb) break semihost_write0
(gdb) continue
(gdb) bt
(gdb) info registers r0 lr
(gdb) x/s $r0
(gdb) info symbol $lr
(gdb) finish
```

**Record** the call chain, `r0`, the string it designates, `lr`, and the
symbol and offset `lr` falls at, then the line at which `finish` stops.
In a call the caller places the first argument in `r0` and the return
address in `lr`; confirm both from what you recorded. `lr` is odd: relate
it to the address `bt` gives for the caller's frame, and to the Thumb bit
of week 2. The function returns no value, so `r0` after `finish` means
nothing.

**Part 2: a one-shot breakpoint.** In the same session:

```text
(gdb) delete
(gdb) tbreak semihost_write0
(gdb) continue
(gdb) info registers r0
(gdb) info breakpoints
```

**Record** `r0` and the output of `info breakpoints`. The argument
differs in kind from Part 1's: the first call passes a string literal and
this one a variable. Place each address in the memory map of week 2 and
account for the difference.

**Part 3: a watched value.** Stop QEMU with `Ctrl-C`, and start again
with `make debug`. In gdb:

```text
(gdb) target remote :1234
(gdb) break main
(gdb) continue
(gdb) watch systick_ctrl
(gdb) continue
```

**Record** the old and the new value, and the line gdb reports. State
which statement changed the variable and which statement executes next.

**Part 4: a write made by the debugger.** Stop QEMU, and start it with
the hook of A2, `make debug QEMU_EXTRA="-d unimp"`. In gdb, after
`target remote :1234`:

```text
(gdb) set {unsigned int}0x40010010 = 1
(gdb) x/wx 0x40010010
(gdb) set {unsigned int}0x40028000 = 3
(gdb) x/wx 0x40028000
(gdb) set {unsigned int}0x40080000 = 0xdeadbeef
(gdb) x/wx 0x40080000
```

**Record**, for each address, the value gdb reads back and the lines
QEMU prints in its own terminal. `lecture-notes/week-03.md`, "Touching a
device that is not there", distinguishes a device QEMU models, a
placeholder for a device it does not implement, and an address with no
device; state which each of the three addresses is, and which observation
told you.

**Checkpoint:** the call recorded with its argument, return address and
symbol; the one-shot breakpoint and its removal recorded; the watched
change recorded and attributed to a statement; three writes recorded,
each classified.

### Done looks like — Part B

- One run that faults without any diagnostic and one that reports a
  diagnostic without faulting, each attributed to the program or to the
  platform.
- The vector table's size in words recorded, every slot decoded, and
  the slots your handlers changed identified.
- The `svc` number located in the instruction's encoding through the
  stacked return address.
- A call followed in the debugger — argument register, return address,
  caller — a variable watched through a change, and three writes made by
  the debugger classified as modelled, placeholder or absent.

---

## Where this goes next

Week 3's lecture (`lecture-notes/week-03.md`) treats devices, interrupts
and system calls: a driver for the serial port, the calling convention,
SysTick and the NVIC, and a system-call layer built on `svc`. Its
central idea is one this laboratory has prepared: **an interrupt is an exception whose source is
a device rather than the program.** The mechanism you exercised here is
the one week 3 drives from peripherals and, for system calls, from the
program itself.

- **A3–A5** had you read the eight words the processor stacks on
  exception entry, and observe the processor enter a handler through
  its slot in the vector table. Week 3's "Exception entry: what the
  hardware saves" describes the same frame, and its `SysTick_Handler`
  and `UART0TX_Handler` are functions of the same kind as yours,
  entered the same way: `SysTick_Handler` through slot 15, and
  `UART0TX_Handler`, IRQ 1, through slot 17.
- **B2** found the table to hold exactly the 16 entries the exceptions
  table lists before "16+ IRQs". Week 3's "The board's interrupt lines"
  numbers device interrupts from 0 and places them in the entries
  **after** those sixteen: IRQ *n* is exception 16 + *n*.
- **A4** showed a handler running privileged while the interrupted code
  was not; an interrupt handler inherits that property, and one handler
  can preempt another. **B3** located the number `n` of `svc #n` in the
  instruction's encoding through the stacked return address. Week 3's
  "Reading the number from the stacked frame" uses exactly that
  procedure to tell one system call from another.
- **A5** faulted on SysTick's control register, `0xE000E010`, written
  from unprivileged code. Week 3's "SysTick: the core timer" programs
  that register from privileged code, to make SysTick interrupt
  periodically.
- **B4** read a call's argument from `r0` and its return address from
  `lr`. Week 3's "The AAPCS: how C calls a function" states that rule,
  and its "Catching an exception frame in gdb" uses the commands B4
  practised, with `finish` and `watch`.
- **B1** separated what the platform does not implement from what the
  program does wrong. Week 3 addresses devices through memory-mapped
  registers, where the same question arises for every device it uses;
  "Touching a device that is not there" gives the means you used here.

The `Makefile` you adapted in A1 and extended in A2 remains yours. A
checkpoint whose `Makefile` lacks a `debug` target is given one by the
step A2 practised.
