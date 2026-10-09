# Week 2 laboratory — the toolchain, end to end

This laboratory covers week 1's topic, the toolchain pipeline, and
extends the work done in class. It also practises two skills that week
2's topic, boot and CPU configuration, relies on: writing a function the
compiler gives no prologue or epilogue, and reading the vector table out
of a linked image. The closing section identifies where each is used.

## Objectives

On completion you will have:

- constructed, from the description in the lecture notes rather than by
  copying, a `Makefile` that compiles, links and runs the week 1 image,
  and be able to account for the dependency relations it expresses;
- developed a routine in ARM assembly in an execution environment that
  permits `printf`, and then transferred it, unchanged, into one that
  does not — accounting for the difference in terms of the hosted and
  freestanding execution environments ISO C defines;
- inspected a linked image with the binary utilities, and located its
  symbols in the memory regions the linker script defines;
- observed the linker reject two distinct classes of memory-budget error
  at build time;
- recovered the initial stack pointer and reset vector from the raw
  bytes of the image, and accounted for the difference between the
  address stored in the vector table and the address of the
  corresponding handler.

## Starting point

[`starter/`](starter/), a prepared copy of [`../week-01/`](../week-01/).
Its `Makefile` has been removed; constructing it is exercise A2. All
other files are complete and unmodified.

## Solutions

Worked solutions to every exercise are provided in
[`SOLUTIONS.md`](SOLUTIONS.md), and the completed code in
[`solution/`](solution/). You are expected to attempt each exercise
before consulting its solution: the evidence an exercise asks you to
gather is the substance of the work, and reading a recorded measurement
does not substitute for taking one.

## Structure

The exercises are divided into two parts. **Part A** is intended for
completion during the laboratory session, where an assistant is
available. **Part B** is designed to be carried out independently
afterwards; it requires no resource beyond the repositories, the
published solutions, and the standard toolchain.

---

## Part A — in the laboratory

### A1. Establish the reference behaviour

Before replacing a build system, observe what it produces.

```sh
cd ../week-01
make run
```

The image prints `toolchain OK`. Exit QEMU with `Ctrl-C`. The
conventional `Ctrl-A` `X` does nothing here: `make run` passes
`-monitor null -serial stdio`, which gives the serial line its own
channel with no monitor multiplexed onto it, and it is that multiplexer
which would otherwise interpret `Ctrl-A`.

> Reference: `lecture-notes/week-01.md` §7 for the role of QEMU and
> semihosting here; `docs/environment-setup.md` if the build does not
> complete.

**Checkpoint:** `toolchain OK` observed, and the shell prompt returned.

### A2. Construct the Makefile

`starter/` contains no `Makefile`. Write one.

The reference is `lecture-notes/week-01.md` **§6**, which presents the
mechanism in general terms "so that the ideas transfer to a Makefile you
write yourself." It supplies the variables, the three rule headers,
automatic variables, pattern rules and phony targets, but not the
completed file. §2 documents the compiler flags and §3 the linker flags.

Proceed in the following order, testing after each step:

1. The variables: `CC`, `BUILD_DIR`, `BIN`, `SOURCES`, `OBJECTS`,
   `CFLAGS`, `LDFLAGS`. `OBJECTS` is derived from `SOURCES` by
   substitution, and a wrong derivation does not announce itself: `make`
   builds the wrong list without complaint. Confirm what the variables
   actually expanded to rather than rereading the text — §6, "Making a
   Makefile report its own state", gives `$(info …)` and a `print-%`
   rule for exactly this.
2. The pattern rule `$(BUILD_DIR)/%.o: %.c`. Its recipe requires
   `$(dir $@)`, `$<` and `$@`; see §6, "Automatic variables".
3. The link rule for `$(BIN)`.
4. `all`, `clean`, and a `.PHONY` declaration.
5. The `run` target. The QEMU invocation comprises four flags, none of
   them deducible; copy it from `../week-01/Makefile`. Reproducing that
   command is not the object of the exercise — the dependency graph is.

Then, from a clean tree:

```sh
cd ../week-02-lab/starter
make        # builds build/scratch-os.axf
make run    # prints toolchain OK
```

> Every recipe line must begin with a literal Tab character rather than
> spaces. The resulting diagnostic names neither whitespace nor the
> character `make` expected; §6, "Diagnosing a delimiter error", takes
> the message apart and gives the command that renders the offending
> line's whitespace, since the defect cannot be seen by reading it.

**Checkpoint:** `make run` prints `toolchain OK` from a `Makefile` you
constructed.

### A3. Develop the routine where `printf` is available

The program built in `starter/hosted/` runs on the same simulated
Cortex-M3, under the same QEMU command, compiled for the same
instruction set as the firmware image. It differs in one respect: it is
linked as a **hosted** program, so the C standard library is present.
Assembly developed here can therefore be checked by printing its
results, which the firmware image does not permit.

The `Makefile` in that directory is provided; constructing a build is
A2's subject, not this exercise's.

`starter/hosted/swap.c` is supplied, builds, and runs. Three pieces are
marked `TODO`; complete them in the order below, rebuilding after each.

**Step 0 — observe the starting state.**

```sh
cd starter/hosted
make run
```

The program prints nothing whatever. It has not failed: it ran to
completion. Establish why before proceeding.

**Step 1 — obtain output.** The program does not call

```c
extern void initialise_monitor_handles( void );
```

which establishes the file descriptors newlib's semihosting standard
I/O writes through. Add the call at the point marked in `main`, and
rebuild. Four lines now appear, all reporting unexchanged values.

Retain this observation. Exercise A4 depends on it.

**Step 2 — the reference implementation.** Complete `swap_c`, exchanging
the two values in C. The first two lines of output should now differ.

**Step 3 — the same operation in assembly.** Complete `swap_asm`.
Its body is declared `__attribute__((naked))`, so the compiler generates
no prologue and no epilogue; the closing `bx lr` is already present for
that reason and must remain the final instruction. Your instructions
belong before it.

The exercise requires three properties of the calling convention,
stated here to the extent it needs them; week 2's lecture treats the
convention in full:

- the first argument is passed in `r0`, the second in `r1`;
- a return value would be passed in `r0`; this function has none;
- because no epilogue is generated, nothing returns on your behalf,
  which is what the supplied `bx lr` accomplishes.

Use `ldr` to load through a pointer and `str` to store through one, with
`r2` and `r3` as scratch registers.

This is the first block in this course containing more than one
instruction, which introduces a requirement the single-instruction
examples do not show. The instructions are C string literals, and C
concatenates adjacent string literals — `"a" "b"` is `"ab"` — so the line
breaks between them exist only in the C source and reach the assembler as
nothing. Each instruction must therefore end with an explicit `\n`, as the
supplied `"bx  lr\n"` already does. Omit it and the assembler receives one
run-together line and reports

```
Error: garbage following instruction -- `ldr r2,[r0]ldr r3,[r1]...'
```

quoting your instructions end to end, against a temporary `.s` file rather
than `swap.c`. `lecture-notes/week-02.md` states the rule; the treatment in
`tutorials/arm-assembly-basics/` §4 explains why `\n\t` is the more common
spelling.

The build emits two diagnostics once the parameters are no longer
referenced in C:

```
warning: unused parameter 'a' [-Wunused-parameter]
warning: unused parameter 'b' [-Wunused-parameter]
```

Both are correct and should be left in place. The assembly does use both
values. Determine what the compiler is able to observe, and account for
the diagnostics on that basis.

> References: `tutorials/arm-assembly-basics/` §4 (`naked`, `volatile`,
> inline assembly) and §5 (the calling convention, with a worked example
> of a standalone assembly function called from C). Optional, but
> directly applicable. `lecture-notes/week-02.md` is the lecture that
> treats the subject formally.

**Checkpoint:** `make run` prints four lines, the C and the assembly
implementation exchanging their values identically; and you can state
why the program printed nothing at step 0.

### A4. Transfer it to the freestanding image

Copy `swap_asm` — unchanged, not rewritten — from `starter/hosted/swap.c`
into `starter/src/main.c`, and call it from `main`.

Attempt first to verify it as you did in A3, with `printf`. Add the
include, the call, and rebuild.

The build does not complete. It is the **link** that fails, so there is
no image to run and therefore no output — the absence of output is a
consequence, not the fault itself:

<!-- verify: expect-failure (fixture a4-printf asserts this) -->

```console
$ make
src/main.c:129:(.text+0x6c): undefined reference to `printf'
collect2: error: ld returned 1 exit status
make: *** [build/scratch-os.axf] Error 1
```

Establish the cause before continuing. The two programs contain the same
function, compiled for the same processor by the same compiler, and are
executed by the same QEMU command. What differs is how they were
**linked**, and therefore which execution environment they run in:

- The hosted program is linked with `--specs=rdimon.specs`, which
  supplies newlib's semihosting C library and its startup code, and it
  calls `initialise_monitor_handles` to establish the descriptors stdio
  writes through.
- The firmware image is compiled `-ffreestanding` and linked with
  `-nostartfiles -nostdlib`, so no C library is searched at all and
  `printf` has no definition to resolve to. Its startup code is
  `boot/startup.c`, written by hand for this course. `Reset_Handler`
  initialises `.data` and `.bss` and calls `main`; it performs none of the
  run-time initialisation stdio would require, and
  `initialise_monitor_handles` is not linked in to be called.

  Note the order in which those two facts matter. The link failing is
  what you observe; the missing run-time initialisation is why supplying
  `printf` would not rescue the attempt anyway. Drop `-nostdlib` and the
  link gets further, only to fail on the syscall stubs newlib's stdio
  needs — `_write`, `_read`, `_sbrk` — which this image also does not
  provide.

ISO C §4 names these two cases: a **hosted** implementation provides the
full standard library, a **freestanding** implementation does not.
`scratch-os` is freestanding by design, which is why the header comment
of `../week-01/src/main.c` records that the image writes through
`semihost_write0` rather than `printf`. Read that comment now; A3 and A4
together are the experiment that demonstrates it.

Compare the two images:

```sh
arm-none-eabi-size hosted/build/swap.axf    # the hosted program
arm-none-eabi-size build/scratch-os.axf     # the firmware image
```

(Exercise B1 adds a `size` target to your `Makefile` so that this need
not be typed in full; invoking the tool directly works now.)

Remove the `printf` attempt. `semihost_write0` accepts a NUL-terminated
string — terminated by the `'\0'` byte, distinct from the null pointer
constant `NULL` — so verify the exchange in C and report a fixed string,
following the convention `toolchain OK` establishes:

```c
int x = 1, y = 2;
swap_asm( &x, &y );
semihost_write0( ( x == 2 && y == 1 ) ? "swap OK\n" : "swap FAILED\n" );
```

**Checkpoint:** `make run` prints `toolchain OK` followed by `swap OK`,
and you can state which property of the link, rather than of the
processor or the source, accounts for the absence of `printf`.

### Done looks like — Part A

- `starter/` builds and runs from a `Makefile` you constructed, and you
  can state the purpose of each of its rules.
- The hosted program prints the results of both the C and the assembly
  implementation, exchanging their values identically.
- `make run` in `starter/` prints `swap OK`, produced by the same
  assembly routine transferred without modification.
- You can state which property of the link, rather than of the processor
  or the source, accounts for `printf` being available in one program
  and not in the other.

Part A is self-contained. Part B requires none of the laboratory's
facilities.

---

## Part B — independent work

### B1. Extend the Makefile, and examine its dependency graph

Add two phony targets:

- `size`, which runs `arm-none-eabi-size` on `$(BIN)`;
- `disasm`, which runs `arm-none-eabi-objdump -d $(BIN)`.

Both name actions rather than files, so both belong in the `.PHONY`
declaration; see §6, "Phony targets".

Then examine the rebuild behaviour. Beginning from a complete build:

1. Invoke `make` again with nothing modified. Which recipes run, and
   why?
2. `touch src/main.c`, then `make`. Which object is recompiled, which is
   not, and does the link run?
3. `touch boot/startup.c`, then `make`. Answer the same three questions.
4. `touch scripts/mps2_m3.ld`, then `make`. This case differs from the
   preceding two; §6 accounts for it near its close, where it notes that
   the linker script is an input to the link but is not declared a
   prerequisite of it.

Record what occurred in each case, with the reason.

**Checkpoint:** `make size` and `make disasm` both function; four
written answers, the fourth identifying the missing prerequisite.

### B2. Compare the generated code with the hand-written code

Two implementations of the same function are now available, both of your
own writing and both already verified in A3: the assembly, now in
`src/main.c`, and the C reference `swap_c` in `hosted/swap.c`.

1. Cross-compile the C implementation so that both target the
   Cortex-M3:
   ```sh
   mkdir -p build
   arm-none-eabi-gcc -mthumb -mcpu=cortex-m3 -O0 -c \
       hosted/swap.c -o build/swap-c.o
   arm-none-eabi-objdump -d build/swap-c.o
   ```
   The object file belongs in `build/`, which `make clean` removes. An
   object left outside the tree survives a clean and a later run can read
   a stale one without noticing.
   Both `swap_c` and `swap_asm` appear in the listing, which is
   convenient: the two are compiled side by side from one source.
2. Disassemble the firmware image with `make disasm` and locate
   `<swap_asm>`, confirming it is identical to the hosted build's.
3. Compare them. Which instructions correspond? Which instructions
   appear only in the generated code, and which of those are accounted
   for by `__attribute__((naked))`?
4. Repeat step 1 with `-Os` in place of `-O0`, writing to
   `build/swap-c-os.o` so both objects remain available, and compare
   again.

> Reference: `tutorials/binutils-and-disassembly/` §3 on reading a
> disassembly listing, §6 on the effect of optimization level.

**Checkpoint:** a written comparison identifying at least one
instruction present in the generated code and absent from the
hand-written code, with the reason.

### B3. A third formulation: extended inline assembly

`__attribute__((naked))` is not the usual means of embedding assembly.
Write a third implementation of `swap` as an ordinary, non-`naked`
function using extended inline assembly with operand constraints:

```c
static void swap_ext( int *a, int *b )
{
    __asm__ volatile (
        /* ... */
        :                        /* outputs  */
        : "r"( a ), "r"( b )     /* inputs   */
        : /* clobbers */ );
}
```

Determine what the clobber list must contain, and whether `"memory"` is
required. Disassemble the result and compare it with both preceding
implementations.

> Reference: `tutorials/arm-assembly-basics/` §4, "Extended asm".

**Checkpoint:** `swap_ext` functions correctly, together with a written
account of the clobber list's contents and the consequence of omitting
them.

### B4. Locate symbols in the map file

If your `Makefile` does not pass `-Xlinker -Map=`, no map file is
produced. Adding it forms part of this exercise; §3 documents the flag.

In `build/output.map`, locate `main`, `isr_vector`, `_sdata` and
`_ebss`. For each, identify the `MEMORY` region defined in
`scripts/mps2_m3.ld` that contains it, and confirm the address falls
within that region's bounds.

Corroborate with `arm-none-eabi-nm -n build/scratch-os.axf`, which
answers the same question in a single listing.

> References: `lecture-notes/week-01.md` §5 on the ELF image and its
> map; `tutorials/binutils-and-disassembly/` §2 on reading `nm` output.

**Checkpoint:** four symbols, four addresses, four regions, and a
statement of which of the two tools is preferable for this question.

### B5. Provoke two link-time failures

The linker script concludes with an `ASSERT` guarding the RAM budget.
Cause it to fail:

1. Increase `_Min_Stack_Size` until the reserved stack meets the heap.
   Build, and record the diagnostic.
2. Separately, reduce the `LENGTH` of `FLASH` until `.text` no longer
   fits. Record that diagnostic, which differs.
3. Revert both modifications.

Both are build failures rather than run-time faults: the linker rejects
at build time a condition that would otherwise corrupt memory during
execution.

> Reference: `lecture-notes/week-01.md` §3.

**Checkpoint:** both diagnostics, transcribed exactly, and a statement
of the difference between them.

### B6. Recover the vector table from the image

The first two words at address zero constitute the boot contract.

```sh
arm-none-eabi-objdump -s -j .isr_vector build/scratch-os.axf
```

1. Take the first four bytes. They are stored little-endian; assemble
   them into a 32-bit word. Which linker symbol does it correspond to?
   Confirm with `nm`.
2. Repeat for the second word, and compare it with the address `nm`
   reports for `Reset_Handler`. The two differ by one. Establish the
   reason before proceeding: it is a property of the architecture, not a
   defect.
3. Examine the remainder of the table. One value recurs. Identify the
   symbol it denotes and account for its frequency.
4. Write an account of the sequence from power-on to the first statement
   of `main`, justifying each step by evidence obtained in this
   laboratory rather than by recollection of the lecture.

> Reference: `tutorials/binutils-and-disassembly/` §4 on raw contents
> and byte order, §5 on the Thumb bit.

**Checkpoint:** both words decoded and identified, the difference of one
accounted for, and the boot sequence recorded.

### Optional extension

Rebuild the firmware image with `-mcpu=cortex-m0` in place of
`-mcpu=cortex-m3`.

**Predict the outcome before testing.** The Cortex-M0 implements
ARMv6-M, a strict subset of the Cortex-M3's ARMv7-M, and a strict subset
excludes instructions.

Then test it, and account for what you observe. If you predicted a
failure, identify the instruction you expected to be rejected and
establish from the disassembly whether this image in fact contains any
instruction outside ARMv6-M.

Finally, consider a later week: week 7 implements a spinlock using
`ldrex` and `strex`. Determine whether that image would survive the same
substitution, and whether the code could be rewritten to avoid the
problem.

### Done looks like — Part B

- `make size` and `make disasm` function, and you can account for the
  consequence of modifying each of four files.
- Three implementations of the exchange — hand-written assembly,
  compiler-generated code, and extended inline assembly — disassembled
  and compared in writing.
- Four symbols located in the map file and corroborated with `nm`.
- Two distinct link-time failures, reproduced and reverted.
- The first two words of the vector table decoded from raw contents, the
  difference of one accounted for, and the boot sequence recorded.

---

## Where this goes next

Week 2's lecture treats boot and CPU configuration: it reimplements
`Reset_Handler` in hand-written assembly, establishes the stack before
any C code executes, and constructs the vector table directly.

Both elements have been encountered here.

- **A3** required a `__attribute__((naked))` function — no prologue, no
  epilogue, and an explicit `bx lr`. `Reset_Handler` is declared the
  same way for a stricter reason: it executes before a stack exists, so
  a compiler-generated prologue saving registers would write through a
  stack pointer that does not yet designate usable memory.
- **B6** required recovering the first two words of the vector table
  from a linked image, and establishing that entry 1 carries the Thumb
  bit while entry 0 does not. The lecture constructs that table
  directly; the difference of one accounted for in B6 is what makes the
  construction correct.

The `Makefile` constructed in A2 remains in use: every subsequent
checkpoint employs the same one.
