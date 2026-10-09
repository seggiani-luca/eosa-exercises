# Week 2 laboratory — worked solutions

Solutions to every exercise are provided here. You are expected to
attempt an exercise before consulting its solution: the evidence an
exercise asks you to gather is the substance of the work, and reading a
recorded measurement does not substitute for taking one.

Task document: [`README.md`](README.md). Completed code:
[`solution/`](solution/).

Every figure, diagnostic and listing below was produced by executing the
stated command against `solution/` using the course toolchain (Arm GNU
Toolchain 15.3, QEMU 11.1). Your addresses should correspond: the linker
script fixes them.

---

## Part A

### A1. Establish the reference behaviour

```console
$ cd ../week-01 && make run
toolchain OK
```

The purpose is to observe the output of the existing build before
replacing the build system that produces it, so that when A2's
`make run` prints the same line the result is attributable to the
`Makefile` you constructed.

### A2. Construct the Makefile

The complete file is [`solution/Makefile`](solution/Makefile), annotated
rule by rule. Rather than reproduce it here, the following are the
elements against which to check your own.

| Element | Function |
|--|--|
| `OBJECTS := $(SOURCES:%.c=$(BUILD_DIR)/%.o)` | derives the object list from the source list, so that adding a source requires one edit rather than two |
| `$(BUILD_DIR)/%.o: %.c` using `$(dir $@)`, `$<`, `$@` | a single rule serving every source file; see §6, "Automatic variables" |
| `$(BIN): $(OBJECTS)` | declares the link's dependence on every object, which is what constitutes the graph |
| `.PHONY: all clean run` | declares these targets to be actions rather than files; without it, a file named `clean` would suppress `make clean` |
| `-Xlinker -Map=$(BUILD_DIR)/output.map` | emits the map file, without which exercise B4 has no input |

**On the diagnostic `missing separator`:** a recipe line began with
spaces. Recipe lines require a literal Tab character. The diagnostic is
the most frequent failure in a first `Makefile` and does not indicate
its cause.

**On the diagnostic `No targets specified and no makefile found`:** the
working directory is wrong, or the file has been saved under a name
other than `Makefile`.

**On ordering:** `all` serves as the default goal because it is the
first target in the file, not by virtue of its name. Placing `clean`
first would cause `make`, invoked without arguments, to delete the
build.

### A3. Develop the routine where `printf` is available

[`solution/hosted/swap.c`](solution/hosted/swap.c).

**Step 0.** The supplied program prints nothing. It does not fail, hang
or crash: it executes every `printf` and terminates normally. The output
is discarded because stdio has no file descriptors to write through.

**Step 1.** The missing call is:

```c
initialise_monitor_handles();
```

placed at the head of `main`. It is supplied by newlib's semihosting
library and appears in no public header, which is why the declaration
must be written by hand. With it added, four lines appear, all reporting
unexchanged values, because the two routines are still empty.

The significance is that omitting it removes output entirely rather than
degrading it. Retain that: A4 turns on it.

**Step 2, the reference implementation**, is the conventional
three-statement form:

```c
void swap_c( int *a, int *b )
{
    int tmp = *a;
    *a = *b;
    *b = tmp;
}
```

The temporary is necessary. The sequence `*a = *b; *b = *a;` destroys
the value of `*a` in its first statement, after which the second assigns
the value of `*b` to itself. If your output presented two equal values,
this is the cause.

**Step 3, the same operation in assembly:**

```c
__attribute__( ( naked ) ) void swap_asm( int *a, int *b )
{
    __asm__ volatile (
        "ldr r2, [r0]\n"   /* r2 = *a */
        "ldr r3, [r1]\n"   /* r3 = *b */
        "str r3, [r0]\n"   /* *a = r3 */
        "str r2, [r1]\n"   /* *b = r2 */
        "bx  lr\n"         /* return; no epilogue is generated */
    );
}
```

Four instructions and a return. Registers `r0` and `r1` already hold the
two pointers on entry, by the calling convention, which is why the
parameter names do not appear in the body. Registers `r2` and `r3` are
call-clobbered and may be used without preservation.

**The `\n` on every line is a requirement, not formatting.** The five
instructions are five C string literals, and C concatenates adjacent
string literals, so without the escapes the assembler receives the single
line `ldr r2, [r0]ldr r3, [r1]str r3, [r0]str r2, [r1]bx  lr` and reports
`Error: garbage following instruction`, quoting that run-together text
against a temporary `.s` file rather than against `swap.c`. The newline
must be inside the string, where the assembler sees it; the line breaks in
the source are consumed by the C compiler. `solution/hosted/swap.c` carries
`\n` on all five lines for this reason. The `\n\t` spelling seen elsewhere
adds only indentation to the generated assembly: compiled both ways, the
machine code is byte-identical.

**The `bx lr` is a frequent omission.** `__attribute__((naked))`
suppresses the epilogue, so no return is generated. Omitting it causes
execution to continue past the end of the function into whatever was
linked next.

```console
$ make run
C   before: x=1 y=2
C   after:  x=2 y=1
asm before: x=1 y=2
asm after:  x=2 y=1
```

**The two diagnostics.**

```
warning: unused parameter 'a' [-Wunused-parameter]
warning: unused parameter 'b' [-Wunused-parameter]
```

Both are correct. The assembly does use the two values, but obtains them
from `r0` and `r1` — within a string literal that the C front end does
not analyse. As far as the compiler can determine, the parameters `a`
and `b` are declared and never referenced. Their names serve to document
the interface and to cause the compiler to establish the call site
correctly; the body's access to them is not visible to it.

The diagnostics should be left in place. Suppressing them would require
C statements within a function declared `__attribute__((naked))`, which
is undefined behaviour and the one construction such a function must not
contain.

### A4. Transfer it to the freestanding image

The routine is copied without modification. The `printf` attempt fails
to link — `undefined reference to 'printf'` — so no image is produced
and nothing runs. The reason is neither the processor nor the source.

**The two programs are the same code.** Disassembling `swap_asm` in each
image yields identical instruction sequences, at different addresses:

```
00000200 <swap_asm>:          <- hosted build
     200:	6802      	ldr	r2, [r0, #0]
     202:	680b      	ldr	r3, [r1, #0]
     204:	6003      	str	r3, [r0, #0]
     206:	600a      	str	r2, [r1, #0]
     208:	4770      	bx	lr
```

Same compiler, same `-mcpu=cortex-m3`, same Thumb encodings, same QEMU
command. The difference is entirely in the link.

| | hosted (`hosted/swap.axf`) | freestanding (`build/scratch-os.axf`) |
|--|--|--|
| C library | newlib, via `--specs=rdimon.specs` | none: `-nostdlib` |
| startup code | newlib's, supplied by the specs file | `boot/startup.c`, written for this course |
| `-nostartfiles` | absent | present |
| stdio initialisation | `initialise_monitor_handles()` | not linked in, so not callable |
| `printf` | writes to the terminal | undefined at link time |

ISO C §4 names these two cases: a **hosted** implementation provides the
full standard library; a **freestanding** implementation does not.
`scratch-os` is freestanding by design. This is precisely what the
header comment of `../week-01/src/main.c` records, and A3 and A4 together
constitute the experiment that demonstrates it.

Two distinct obstacles stand between this image and `printf`, and it is
worth keeping them apart. The one you meet is the **link**: `-nostdlib`
means no C library is searched, so the symbol is never resolved. Behind
it stands a second, which you would meet only if the first were removed:
newlib's semihosting stdio expects C run-time initialisation, and
`initialise_monitor_handles` in particular, that the hand-written
`Reset_Handler` never performs. Supplying the library is therefore not
sufficient either. The exercise stops at the first because that is where
the toolchain stops.

**What the standard library costs.**

```console
$ arm-none-eabi-size hosted/build/swap.axf
   text	   data	    bss	    dec	    hex	filename
  45864	   2520	   5568	  53952	   d2c0	hosted/build/swap.axf

$ arm-none-eabi-size build/scratch-os.axf
   text	   data	    bss	    dec	    hex	filename
    436	      0	   4096	   4532	   11b4	build/scratch-os.axf
```

The tool is invoked directly because the firmware `Makefile` has no
`size` target at this point; exercise B1 adds one.

45,864 bytes of code against 436: a factor of roughly 105, for one
`printf`. On a part with a few tens of kilobytes of FLASH the hosted
image does not fit at all. This is the substantive reason embedded
images are freestanding, rather than a limitation of the course's
scaffolding.

**Verification without printing.** `semihost_write0` accepts a
NUL-terminated string — terminated by the `'\0'` byte, distinct from the
null pointer constant `NULL` — so the exchange is checked in C and
reported as a fixed string:

```c
int x = 1, y = 2;
swap_asm( &x, &y );
semihost_write0( ( x == 2 && y == 1 ) ? "swap OK\n" : "swap FAILED\n" );
```

```console
$ make run
toolchain OK
swap OK
```

This follows the convention established by `toolchain OK` and continued
by week 4's `partitions OK`.

---

## Part B

### B1. Extend the Makefile, and examine its dependency graph

```make
.PHONY: all clean run size disasm

size: $(BIN)
	arm-none-eabi-size $(BIN)

disasm: $(BIN)
	arm-none-eabi-objdump -d $(BIN)
```

Each declares `$(BIN)` as a prerequisite, so `make disasm` on a clean
tree builds the image first. Both belong in the `.PHONY` declaration.

```console
$ make size
   text	   data	    bss	    dec	    hex	filename
    436	      0	   4096	   4532	   11b4	build/scratch-os.axf
```

`bss` is 4096, the reserved heap, although nothing in the program
allocates. `data` is 0, no initialised global variable being present.

**The four cases.**

1. **`make` with nothing modified** produces `make: Nothing to be done
   for 'all'.` Every target is newer than its prerequisites, so no
   recipe runs. This states the mechanism precisely: `make` compares
   modification times; it does not determine intent.

2. **`touch src/main.c`** recompiles that source alone, then links:

   ```
   arm-none-eabi-gcc ... -c src/main.c -o build/src/main.o
   arm-none-eabi-gcc ... build/boot/startup.o build/src/main.o -o build/scratch-os.axf
   ```

   `build/boot/startup.o` is not rebuilt, its prerequisite being
   unchanged. The link runs because one of its own prerequisites has
   changed.

3. **`touch boot/startup.c`** produces the converse: that source alone
   is recompiled, followed by the link, for the same reason.

4. **`touch scripts/mps2_m3.ld`** produces `make: Nothing to be done for
   'all'.`

   **This case is the instructive one, and it records a genuine
   defect.** The linker script is supplied to the link command through
   `-T`, but it is not declared a prerequisite of `$(BIN)`, so `make`
   has no basis for concluding that the image depends on it. A
   modification to the memory map will therefore leave `make` reporting
   the image current while it retains the previous layout.

   `lecture-notes/week-01.md` §6 records this and prescribes
   `make clean && make` after any modification to the linker script. The
   correction is to declare the dependence:

   ```make
   $(BIN): $(OBJECTS) scripts/mps2_m3.ld
   ```

   It is left uncorrected in the course `Makefile` deliberately, so that
   this exercise has a genuine finding. **This bears on B5**, in which
   the linker script is modified repeatedly; use `make clean` there.

### B2. Compare the generated code with the hand-written code

Four listings of the same function follow.

**Hand-written, `__attribute__((naked))` — five instructions:**

```
000000bc <swap_asm>:
  bc:	6802      	ldr	r2, [r0, #0]
  be:	680b      	ldr	r3, [r1, #0]
  c0:	6003      	str	r3, [r0, #0]
  c2:	600a      	str	r2, [r1, #0]
  c4:	4770      	bx	lr
```

**Generated from `swap_c` at `-O0` — twenty instructions:**

```
00000000 <swap_c>:
   0:	b480      	push	{r7}
   2:	b085      	sub	sp, #20
   4:	af00      	add	r7, sp, #0
   6:	6078      	str	r0, [r7, #4]
   8:	6039      	str	r1, [r7, #0]
   a:	687b      	ldr	r3, [r7, #4]
   c:	681b      	ldr	r3, [r3, #0]
   e:	60fb      	str	r3, [r7, #12]
  10:	683b      	ldr	r3, [r7, #0]
  12:	681a      	ldr	r2, [r3, #0]
  14:	687b      	ldr	r3, [r7, #4]
  16:	601a      	str	r2, [r3, #0]
  18:	683b      	ldr	r3, [r7, #0]
  1a:	68fa      	ldr	r2, [r7, #12]
  1c:	601a      	str	r2, [r3, #0]
  1e:	bf00      	nop
  20:	3714      	adds	r7, #20
  22:	46bd      	mov	sp, r7
  24:	bc80      	pop	{r7}
  26:	4770      	bx	lr
```

The instructions present only in the generated code, and their causes:

| Additional instructions | Cause |
|--|--|
| `push {r7}`, `pop {r7}`, `sub sp, #20`, `adds r7, #20`, `mov sp, r7` | the prologue and epilogue, which `__attribute__((naked))` suppresses |
| `str r0, [r7, #4]`, `str r1, [r7, #0]` and the subsequent reloads | the parameters spilled to the stack at `-O0`; an optimization-level effect, not attributable to `naked` |
| `str r3, [r7, #12]` and its reload | the C variable `tmp` allocated a stack slot, as every variable is at `-O0` |

`__attribute__((naked))` therefore accounts for approximately half the
difference, and the optimization level for the remainder. The following
listing establishes this.

**Generated from `swap_c` at `-Os` — five instructions:**

```
00000000 <swap_c>:
   0:	6803      	ldr	r3, [r0, #0]
   2:	680a      	ldr	r2, [r1, #0]
   4:	6002      	str	r2, [r0, #0]
   6:	600b      	str	r3, [r1, #0]
   8:	4770      	bx	lr
```

**This is the hand-written implementation.** The same four memory
operations occur in the same order, followed by the same return; only
the assignment of `r2` and `r3` differs, which is immaterial. Twenty
instructions have become five.

The conclusion is not that hand-written assembly is superior. It is that
the compiler, permitted to optimize, arrives at precisely the four
instructions you selected. What `__attribute__((naked))` provides is not
better code but determinate code — which is why it is applied to reset
handlers and context switches, where the exact instruction sequence
constitutes the requirement, and not to arithmetic.

> Both listings come from separate compilations of `hosted/swap.c`, into
> `build/swap-c.o` at `-O0` and `build/swap-c-os.o` at `-Os`. Neither is
> the build that `solution/` contains, which remains at `-O0`, as every
> checkpoint in this course does. Both objects live under `build/` so that
> `make clean` removes them; an object written outside the tree survives a
> clean, and a later run can read a stale one without noticing.

### B3. A third formulation: extended inline assembly

```c
static void swap_ext( int *a, int *b )
{
    __asm__ volatile (
        "ldr r2, [%0]\n"
        "ldr r3, [%1]\n"
        "str r3, [%0]\n"
        "str r2, [%1]\n"
        :                            /* no outputs                    */
        : "r"( a ), "r"( b )         /* inputs: any general register  */
        : "r2", "r3", "memory" );    /* clobbers                      */
}
```

`%0` and `%1` denote the first and second operands; the compiler selects
the registers they occupy and substitutes the names. No `bx lr` appears:
this function is not declared `__attribute__((naked))`, so the compiler
generates the epilogue, and an explicit return would cause the function
to return twice.

**The clobber list.** `"r2"` and `"r3"` are declared because the block
overwrites them, and the compiler must not hold live values there.
`"memory"` is declared because the block writes through pointers by
means of an instruction sequence the compiler does not analyse. Without
it, the compiler is entitled to assume `*a` and `*b` unchanged across
the block and to retain stale values in registers — a defect that
manifests only at optimization levels at which it has a cached value to
retain. That failure mode constitutes the argument for the constraint
syntax over hard-coded register names: it permits the damage to be
declared.

The disassembly comprises sixteen instructions:

```
000000c8 <swap_ext>:
  c8:	b480      	push	{r7}
  ca:	b083      	sub	sp, #12
  cc:	af00      	add	r7, sp, #0
  ce:	6078      	str	r0, [r7, #4]
  d0:	6039      	str	r1, [r7, #0]
  d2:	6879      	ldr	r1, [r7, #4]
  d4:	6838      	ldr	r0, [r7, #0]
  d6:	680a      	ldr	r2, [r1, #0]
  d8:	6803      	ldr	r3, [r0, #0]
  da:	600b      	str	r3, [r1, #0]
  dc:	6002      	str	r2, [r0, #0]
  de:	bf00      	nop
  e0:	370c      	adds	r7, #12
  e2:	46bd      	mov	sp, r7
  e4:	bc80      	pop	{r7}
  e6:	4770      	bx	lr
```

The four instructions written appear at `d6`–`dc`, enclosed by the `-O0`
prologue, parameter spills and epilogue. The three formulations
therefore differ as follows: `__attribute__((naked))` yields exactly the
instructions written; extended inline assembly yields those instructions
together with the compiler's bookkeeping; and C yields whatever the
optimizer determines.

### B4. Locate symbols in the map file

Add `-Xlinker -Map=$(BUILD_DIR)/output.map` to `LDFLAGS` if it is
absent.

From `build/output.map` and, more directly, from `nm`:

```console
$ arm-none-eabi-nm -n build/scratch-os.axf
00000000 R isr_vector
00000048 T Reset_Handler
000000e8 T main
20000000 B _ebss
20000000 D _sdata
20400000 R _estack
```

| Symbol | Address | Region | Within bounds |
|--|--|--|--|
| `isr_vector` | `0x00000000` | FLASH (`0x0`, 4 MiB) | yes, at its base, as required |
| `main` | `0x000000e8` | FLASH | yes |
| `_sdata` | `0x20000000` | RAM (`0x20000000`, 4 MiB) | yes, at its base |
| `_ebss` | `0x20000000` | RAM | yes |

Two observations follow.

**`_sdata` and `_ebss` hold the same address.** Both `.data` and `.bss`
are empty: the program declares no initialised and no zero-initialised
global variable. The `.data` copy loop and the `.bss` zero loop in
`startup.c` therefore execute zero iterations. They are not dead code;
declaring one global variable causes both to perform work.

**The two address ranges correspond to the two `MEMORY` regions** as
`mps2_m3.ld` declares them. This is the check worth retaining: a symbol
placed outside its region indicates that the linker script and the
expectation of the program have diverged.

**Choice of tool.** `nm -n` answers the question of placement in a
single sorted listing. The map file answers the question of cause,
recording which input section of which object contributed each
constituent:

```
.isr_vector     0x00000000       0x40
 *(.isr_vector)
 .isr_vector    0x00000000       0x40 build/boot/startup.o
```

### B5. Provoke two link-time failures

Use `make clean && make` in both cases; B1, case 4 accounts for why
modifying the linker script alone does not cause a rebuild.

**1. Increase the reserved stack until it meets the heap:**
`_Min_Stack_Size = 0x400000;`

```
ld: region RAM overflowed: heap collides with the stack
collect2: error: ld returned 1 exit status
```

**2. Reduce FLASH below the size `.text` requires:**
`FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 128`

```
ld: section `.text' will not fit in region `FLASH'
ld: region `FLASH' overflowed by 308 bytes
```

**The distinction.** The first diagnostic is our own: it is the string
supplied to the `ASSERT` at the close of `mps2_m3.ld`, and it exists
only because that check was written. Removing the `ASSERT` causes the
link to succeed, producing an image whose stack grows into its heap
during execution, without diagnosis.

The second is the linker's own, emitted for any `MEMORY` region that
overflows, without provision on our part, and reporting the extent of
the overflow.

The common property is that both are build failures. A memory-budget
error detected at link time costs a rebuild; the same error detected
during execution costs a corrupted stack on physical hardware, with no
diagnostic at all.

Both modifications are recorded as a comment block in
[`solution/scripts/mps2_m3.ld`](solution/scripts/mps2_m3.ld) rather than
applied, so that the folder continues to link.

### B6. Recover the vector table from the image

```console
$ arm-none-eabi-objdump -s -j .isr_vector build/scratch-os.axf

Contents of section .isr_vector:
 0000 00004020 49000000 41000000 41000000  ..@ I...A...A...
 0010 41000000 41000000 41000000 00000000  A...A...A.......
```

**1. The first word.** The bytes `00 00 40 20`, stored little-endian,
constitute the word `0x20400000`. `nm` reports `20400000 R _estack`.
This is entry 0: the initial stack pointer, loaded by hardware before
any instruction executes. The value is the base of RAM plus its length
of 4 MiB — the top of the region, the stack growing downwards.

**2. The second word.** The bytes `49 00 00 00` constitute
`0x00000049`. However:

```
00000048 T Reset_Handler
```

The address is `0x48`, not `0x49`. **The table differs by one, and is
correct.**

Under ARMv7-M the low bit of a branch target address does not form part
of the address. It selects the instruction set in which execution
continues: set denotes Thumb, clear denotes ARM. The Cortex-M3 executes
Thumb exclusively, so every code address supplied to the processor as a
branch target must have that bit set. Clearing it causes a fault on the
first instruction.

Entry 0 is not adjusted, being data — a stack pointer rather than a
branch target.

**3. The recurring value.** `41000000` constitutes `0x41`, which is
`Default_Handler` at `0x40` with the same bit set. Every vector for
which no handler has yet been provided refers to it, so that an
unexpected exception enters a defined loop rather than executing
whatever bytes occupy address zero. The entries reading `00000000` are
the slots the architecture reserves.

**4. The boot sequence, from evidence.**

1. On reset, the core loads the word at `0x00000000` into `SP`. That
   word was read above as `0x20400000`, the top of RAM. *(Evidence: the
   section contents, and `_estack` in `nm`.)*
2. The core loads the word at `0x00000004`, clears its low bit, and sets
   `PC`. The word read was `0x49`, so execution commences at `0x48` in
   Thumb state. *(Evidence: the section contents; `Reset_Handler` at
   `0x48` in `nm`.)*
3. `Reset_Handler` copies `.data` from FLASH to RAM and zeroes `.bss`.
   Both loops execute zero iterations here, `_sdata` and `_ebss` being
   equal. *(Evidence: B4's table.)*
4. It calls `main` at `0x000000e8`. *(Evidence: `nm`, and the `bl`
   instruction in the disassembly of `Reset_Handler`.)*
5. `main` calls `semihost_write0`, which executes `bkpt 0xab`. QEMU
   intercepts it, reads `r0` and `r1`, and writes the string to the
   terminal. *(Evidence: `make disasm`, and the observed output.)*

Note what the sequence does not contain: no bootloader, no operating
system, no loader, and no dynamic linking. The image occupies its final
addresses before the first instruction executes.

### Optional extension: `-mcpu=cortex-m0`

**The build succeeds**, and the resulting image runs correctly under the
same QEMU command, printing `toolchain OK` and `swap OK` as before. If
you predicted a failure, that was the reasonable prediction, and the
result is the instructive part.

Nothing in this image lies outside ARMv6-M. `ldr`, `str` and `bx lr` in
their low-register forms are all in the subset, as is `bkpt`, and so is
everything the compiler generated for `startup.c` and `src/main.c` at
this optimization level. A strict subset excludes instructions; this
program happens to use none of the excluded ones. That is a property of
this particular program, not a general result.

**Week 7 is where the substitution stops being harmless.** The spinlock
implemented there is built on `ldrex` and `strex`, the exclusive-access
pair, which ARMv6-M does not provide. The code cannot be rewritten to
avoid the problem: the atomicity belongs to the instruction pair, not to
the algorithm, so no sequence of ARMv6-M instructions reproduces the
guarantee. This is a genuine constraint on which Cortex-M devices can
host which kernel, rather than a property of the toolchain.

**One further observation.** A successful build does not establish that
the image would run on Cortex-M0 hardware. It was executed on QEMU's
`mps2-an385`, which models a Cortex-M3. Compiling for a subset and then
running on the superset tests neither the subset nor the device.
