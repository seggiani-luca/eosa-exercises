# Week 4 laboratory — worked solutions

Solutions to every exercise are provided here. You are expected to
attempt an exercise before consulting its solution, and to read the
hints in [`HINTS.md`](HINTS.md) before this file: the values an
exercise asks you to observe are the substance of the work, and reading
an observed value does not substitute for taking one.

Where an exercise leaves the register operations to you, its solution
follows the course of the work: the first attempt that the exercise text
leads to, what that attempt does, how the cause is found, and then the
working code. The symptoms were produced by running those attempts.

Task document: [`README.md`](README.md). Finished code of the exercises
whose result you keep: [`solution/`](solution/). It holds A2, A3, B2,
B3, B4, B5 and B6 together. A4, B1 and B8 are experiments you make in a
copy; each is given below as the change to apply to `solution/`. The
defective drivers of B7 are in [`broken/`](broken/), one replacement
file each.

Every value, output and listing below was produced by executing the
stated command using the course toolchain (Arm GNU Toolchain 15.3,
QEMU 11.1): against `solution/`, or, for an experiment, against
`solution/` with the stated change applied. Each observed value is also
classified by what fixes it:

- **architectural** — the ARMv7-M architecture defines it; any
  Cortex-M3 produces it;
- **this build** — it depends on the code the compiler generated or on
  how fast the emulator runs, so your value may differ; the relation the
  exercise asks about holds regardless.

---

## Part A

### A1. A known-good baseline

`make run` in the copy of `week-03/solution/` prints the lines the
checkpoint lists, then one `uptime` line a second. `Ctrl-C` ends it: the
`Makefile` starts QEMU with `-monitor null -serial stdio`, which gives
the serial line no escape sequence of its own, so `Ctrl-A` `X` reaches
the program as two typed characters (it prints `key: x`).

Fix any difference now: every later exercise assumes this baseline, and
a defect inherited from here looks like a defect in the new code.

### A2. Interrupt-driven UART receive

The two natural first attempts both fail, in different ways. They were
run on a copy of `solution/` without the watchdog start-up of B3, which
you do not have yet.

**First attempt: the handler, the ring buffer and `RXIRQEN`, but no
enable in the NVIC.** Typing keys prints nothing. No `key:` line
appears while the `uptime` lines continue: the system is running and
deaf. In gdb a breakpoint on `UART0RX_Handler` is never hit. The NVIC
shows why. With a key typed and the program stopped by
`break *SysTick_Handler`, `p/x NVIC->ISER[0] & 1` gives `0x0` and
`p/x NVIC->ISPR[0] & 1` gives `0x1`: the device raised the line, the NVIC
holds the request as pending, and nothing delivers it because the
interrupt is not enabled there. The interrupt has two enables, one in
the device and one in the NVIC, and the priority is set before the
second.

**Second attempt: the NVIC enabled, `INTSTATUS` not written.** The first
key stops the system. The `uptime` lines end at the second before the
key and no `key:` line is printed, although the handler is running. In
gdb, `break *UART0RX_Handler`, `ignore 1 50` and `continue` return
almost at once, `info breakpoints` shows the breakpoint hit more than 50
times, and `p/x CMSDK_UART0->INTSTATUS & 2` gives `0x2`: the device's
receive flag is still set. The NVIC lines of this board are
level-sensitive, so the line stays active until the device's own flag is
cleared, and the handler is entered again as soon as it returns. Its
priority, `0xC0`, is more urgent than SysTick's, `0xE0`, so SysTick is
never taken again and the application never runs. The handler prints
nothing, which is why the output simply stops. The repair is to
acknowledge at the device: write the bit to `INTSTATUS` (write 1 to
clear).

**The working driver.** The receive buffer has the shape of the send
buffer, with the roles of the two indices exchanged: the handler writes
`rx_head`, the reader writes `rx_tail`.

```c
#define RX_BUF_SIZE 32
static char rx_buf[RX_BUF_SIZE];
static volatile unsigned rx_head;
static volatile unsigned rx_tail;

void UART0RX_Handler( void )
{
    char c = (char) ( CMSDK_UART0->DATA & 0xFF );

    unsigned next = ( rx_head + 1 ) & ( RX_BUF_SIZE - 1 );
    if ( next != rx_tail )
    {
        rx_buf[rx_head] = c;
        rx_head = next;
    }

    CMSDK_UART0->INTSTATUS = UART_INT_RX;
}

int uart_getc( void )
{
    int c = -1;
    uint32_t saved = irq_save();
    if ( rx_head != rx_tail )
    {
        c = (unsigned char) rx_buf[rx_tail];
        rx_tail = ( rx_tail + 1 ) & ( RX_BUF_SIZE - 1 );
    }
    irq_restore( saved );
    return c;
}
```

`uart_init` adds `UART_CTRL_RXIRQEN` to `CTRL`, sets
`NVIC->IPR[UART0RX_IRQn] = 0xC0`, then sets the bit in `NVIC->ISER`.
The priority is written before the enable, so the interrupt can never
be taken at a priority nobody chose. `sys_read_impl` returns
`uart_getc()`.

`uart_getc` blocks interrupts while it moves `rx_tail`, because the
handler reads `rx_tail` to decide whether the buffer is full: without
the protection, an interrupt between the comparison and the update would
act on a stale index.

Typing ten keys one after another gives ten `key:` lines, and the
`uptime` lines continue. With `break *UART0RX_Handler` and `continue` in
gdb, the first key typed in the QEMU terminal stops the program at
`Breakpoint 1, UART0RX_Handler`. Type the keys as you normally would. A whole string written to QEMU's
input in a single burst printed only its first key here until further
input arrived; that is a property of how the emulator is fed, observed
with this driver, and it does not appear when keys are typed.

### A3. A timer driver

**First attempt: a handler that counts and prints, without the
acknowledge.** Within six seconds the output holds more than a hundred
`timer0:` lines and no `uptime` line. In gdb, `break *TIMER0_Handler`,
`ignore 1 50` and `continue` return almost at once, the breakpoint is
hit more than 50 times, and `p/x CMSDK_TIMER0->INTSTATUS` gives `0x1`:
the timer's flag is still set. The cause is that of the second attempt
at A2. The handler's priority, `0xC0`, is more urgent than SysTick's,
`0xE0`, so SysTick is never taken again: the clock stops, and with it
the `uptime` lines. The repair is to write `TIMER_INT_PENDING` to
`INTSTATUS` first.

**The working driver.**

```c
void timer0_init( uint32_t period_ms )
{
    CMSDK_TIMER0->RELOAD = 25000UL * period_ms;
    CMSDK_TIMER0->CTRL = TIMER_CTRL_EN | TIMER_CTRL_IRQEN;
    NVIC->IPR[TIMER0_IRQn] = 0xC0;
    NVIC->ISER[TIMER0_IRQn / 32] = 1UL << ( TIMER0_IRQn % 32 );
}

void TIMER0_Handler( void )
{
    CMSDK_TIMER0->INTSTATUS = TIMER_INT_PENDING;
    ticks++;
    if ( ticks % 10 == 0 )
    {
        kprintf( "timer0: %u\n", ticks );
    }
}
```

The timer counts down at the 25 MHz processor clock, so 25 000 cycles
make a millisecond, and a 100 ms period gives ten ticks a second. The
output is:

```
timer0: 10
uptime: 1000 ms
timer0: 20
uptime: 2000 ms
```


### A4. Preemption

In a copy of your working folder. The changes to `drivers/timer.c`: the
priority of `TIMER0` becomes `0x40`, its handler reports whether the
slow handler is running, and `TIMER1` is added.

```c
static volatile int in_slow;

void TIMER0_Handler( void )
{
    CMSDK_TIMER0->INTSTATUS = TIMER_INT_PENDING;
    if ( in_slow )
    {
        kprintf( "  T0 (inside T1)\n" );
    }
    else
    {
        kprintf( "T0\n" );
    }
}

void timer1_init( void )
{
    CMSDK_TIMER1->RELOAD = 25000000UL;          /* 1 s */
    CMSDK_TIMER1->CTRL = TIMER_CTRL_EN | TIMER_CTRL_IRQEN;
    NVIC->IPR[TIMER1_IRQn] = 0xE0;
    NVIC->ISER[TIMER1_IRQn / 32] = 1UL << ( TIMER1_IRQn % 32 );
}

void TIMER1_Handler( void )
{
    CMSDK_TIMER1->INTSTATUS = TIMER_INT_PENDING;
    in_slow = 1;
    kprintf( "T1 start\n" );
    for ( volatile uint32_t i = 0; i < 100000000; i++ )
    {
    }
    kprintf( "T1 end\n" );
    in_slow = 0;
}
```

`timer1_init()` is called from `main()` after `timer0_init( 100 )`. With
the loop count above, each `T1` took about 300 ms here (this build and
this machine; adjust the count). The output:

```
T1 start
  T0 (inside T1)
  T0 (inside T1)
  T0 (inside T1)
T1 end
uptime: 1010 ms
```

`TIMER0` fires every 100 ms and is more urgent than `TIMER1`, so it
preempts the handler three times. No `uptime` line is printed between
`T1 start` and `T1 end`: the application runs below every handler. The
uptime count itself stops advancing for the same reason: SysTick has
the priority of `TIMER1`, `0xE0`, and an exception of equal priority
does not preempt a running handler, so SysTick waits, pending, until
`T1` returns.

In gdb, after `break *TIMER1_Handler` and `continue`, the low nine bits
of xPSR are the number of the active exception and `lr` holds
`EXC_RETURN`. Both are architectural:

```
(gdb) p/x $xpsr & 0x1ff
$1 = 0x19
(gdb) p/x $lr
$2 = 0xfffffff9
```

`0x19` is 25: 16 plus `TIMER1_IRQn`, 9. `0xfffffff9` returns to thread
mode on the main stack: `TIMER1_Handler` was entered from `sleep_ms`.
After `delete`, `break *TIMER0_Handler`, `continue`:

```
(gdb) p/x $xpsr & 0x1ff
$3 = 0x18
(gdb) p/x $lr
$4 = 0xfffffff1
(gdb) bt
#0  TIMER0_Handler () at drivers/timer.c:34
#1  <signal handler called>
#2  0x000004b2 in TIMER1_Handler () at drivers/timer.c:59
#3  <signal handler called>
#4  sleep_ms (ms=10) at lib/printf.c:93
```

`0x18` is 24, `16 + 8`. `0xfffffff1` returns to **handler** mode: the
interrupted code is `TIMER1_Handler` itself, and `bt` shows it below.
The addresses and line numbers are **this build**.

Swapping the two priorities, `TIMER0` at `0xE0` and `TIMER1` at `0x40`:
`TIMER0` is no longer more urgent, so it waits until `TIMER1_Handler`
returns, and no `T0` appears inside `T1`. SysTick, at `0xE0`, is less
urgent than `TIMER1`, so it waits too, and `uptime` stalls for as long
as `T1` runs.

---

## Part B

### B1. A race, and its fix

In a copy. `TIMER0` gets a 1 ms period (`timer0_init( 1 )`), its
handler adds `counter++` and `isr_hits++` after the acknowledge, and
`main()` runs, before the privilege drop:

```c
for ( int i = 0; i < 100000; i++ )
{
    uint32_t tmp = counter;
    for ( volatile int j = 0; j < 100; j++ )
    {
    }
    counter = tmp + 1;
}
CMSDK_TIMER0->CTRL = 0;
kprintf( "counter=%u expected=%u diff=%d\n", counter, 100000 + isr_hits,
         (int) ( 100000 + isr_hits - counter ) );
```

Without the protection the difference is not zero:
`counter=100001 expected=100025 diff=24` in one run, `diff=28` in
another. Each lost update is an interrupt that fell between the load
into `tmp` and the store of `tmp + 1`: the store overwrote the
handler's increment. The count varies from run to run (**this build**).
The delay loop widens the gap; without it QEMU, which translates
instructions in groups and checks for interrupts between groups, would
rarely show a loss.

With `uint32_t s = irq_save();` before `tmp = counter` and
`irq_restore( s );` after the store, every run prints `diff=0`.
`irq_save` returns the previous mask and `irq_restore` puts it back, so
the pair is correct also where interrupts were already masked, which a
plain `cpsid i` / `cpsie i` is not: `cpsie i` would enable them
unconditionally. The experiment runs privileged because `cpsid i` has
no effect in unprivileged code.

### B2. One driver, two UARTs (optional)

```c
void uart_putc_polled_on( CMSDK_UART_TypeDef *u, char c )
{
    while ( u->STATE & UART_STATE_TXBF )
    {
    }
    u->DATA = (uint32_t) (unsigned char) c;
}

void uart_putc_polled( char c )
{
    uart_putc_polled_on( CMSDK_UART0, c );
}

void uart_init_on( CMSDK_UART_TypeDef *u )
{
    u->BAUDDIV = 16;
    u->CTRL = UART_CTRL_TXEN | UART_CTRL_RXEN;
}
```

`main()` calls `uart_init_on( CMSDK_UART1 )` and sends `hello uart1\n`
one byte at a time with `uart_putc_polled_on`, before the privilege
drop. Run with the second serial option, and after quitting:

```
$ cat uart1.txt
hello uart1
```

### B3. The watchdog

```c
void wdog_init( void )
{
    CMSDK_WDOG->LOCK = WDOG_UNLOCK_KEY;
    CMSDK_WDOG->LOAD = 25000000UL;
    CMSDK_WDOG->CTRL = WDOG_CTRL_INTEN | WDOG_CTRL_RESEN;
    CMSDK_WDOG->LOCK = 0;
}

void wdog_feed( void )
{
    CMSDK_WDOG->LOCK = WDOG_UNLOCK_KEY;
    CMSDK_WDOG->INTCLR = 1;
    CMSDK_WDOG->LOCK = 0;
}

void NMI_Handler( void )
{
    kprintf( "watchdog: NMI\n" );
}
```

`wdog_init` is called from `main()` before the drop and `wdog_feed`
from `SysTick_Handler`. `NMI_Handler` replaces the weak default of the
vector table. The `LOCK` register ignores writes to the others while
locked: the key unlocks, any other value locks again.

`solution/` feeds the watchdog. Not feeding it (remove the call from
`SysTick_Handler`) gives, in the order printed:

```
scratch-os: kernel started
dropping to unprivileged mode
app: hello from unprivileged code
watchdog: NMI
timer0: 10
uptime: 1000 ms
scratch-os: kernel started
```

The counter reaches 0 after one second and raises NMI; it reloads and
starts again; the second time nobody wrote `INTCLR` before it reached 0,
so `RESEN` resets the board and the banner prints again. The cycle
repeats. Fed every 10 ms, the watchdog never expires and no NMI
appears. A `for ( ;; ) { }` in `SysTick_Handler` stops the feeding, so
the watchdog catches the stuck system, which is its purpose: the check
lives in hardware, outside the code that may hang.

### B4. Board LEDs

`FPGAIO->LED = 0x3;` in `main()`, before the drop. In gdb, stopped by
`break *SysTick_Handler` and `continue`:

```
(gdb) x/wx 0x40028000
0x40028000:	0x00000003
(gdb) p/x FPGAIO->LED
$1 = 0x3
(gdb) set var FPGAIO->LED = 0xff
(gdb) p/x FPGAIO->LED
$2 = 0x3
```

Both reads show `0x3`. The write of `0xff` leaves `0x3`: the register
has two bits, so only they are stored.

You stopped in a handler first for a reason that concerns the debugger,
not the processor. gdb takes the definition of the macro `FPGAIO` from
the source file of the frame it is stopped in, and `src/app.c` does not
include `cmsdk.h`. Stopped in `app_main`, which runs unprivileged
(`p/x $control` gives `0x1`), `x/wx 0x40028000` still gives
`0x00000003`, because a device register can be read from unprivileged
code on this board, but `p/x FPGAIO->LED` answers
`No symbol "FPGAIO" in current context`. In `SysTick_Handler`, whose
file includes `cmsdk.h`, the name resolves. The system control space is
different: in `app_main`, `x/wx 0xE000E010` (the SysTick control
register) answers `Cannot access memory at address 0xe000e010`.

### B5. GPIO seen from the bus

```c
void gpio_set_output( unsigned pin )
{
    CMSDK_GPIO0->OUTENABLESET = 1UL << pin;
}

void gpio_write( unsigned pin, int level )
{
    if ( level )
    {
        CMSDK_GPIO0->DATAOUT |= 1UL << pin;
    }
    else
    {
        CMSDK_GPIO0->DATAOUT &= ~( 1UL << pin );
    }
}
```

`make run-trace` prints, right after `scratch-os: kernel started`:

```
cmsdk-ahb-gpio: unimplemented device write (size 4, offset 0x010, value 0x00000001)
cmsdk-ahb-gpio: unimplemented device read  (size 4, offset 0x004)
cmsdk-ahb-gpio: unimplemented device write (size 4, offset 0x004, value 0x00000001)
```

Line one is `OUTENABLESET = 1 << 0`: offset `0x010`, a write, one
access: the set register changes only its own bit. Lines two and three
are `DATAOUT |= 1`: offset `0x004`, a read, then a write of the result.
After adding `gpio_write( 1, 1 )`, the second `DATAOUT` write has value
`0x00000002`, not `0x00000003`: the read returned 0, so the OR started
from 0 and pin 0 was turned off again.

Reads return 0 because QEMU has no model for this block: it holds only
a placeholder that logs the access and answers 0. A model that kept the
latch would return it, and the result would be `0x3`. Set/clear
registers avoid the problem altogether: one write changes one bit, so
nothing is read and nothing can be lost, including when an interrupt
changes the same register between a read and a write.

### B6. Add a system call

**What omitting each place gives.** A new service touches four places,
and leaving one out gives four different messages:

- the number and prototype in `lib/syscalls.h` left out: the
  compiler stops at the call in `src/app.c` with `implicit declaration
  of function 'sys_led'`;
- the stub in `lib/syscalls.s` left out: the compiler is satisfied and
  the link fails with `undefined reference to 'sys_led'`;
- the function that does the work left out, with its table entry still
  present: `kernel/syscall.c` fails with `'sys_led_impl' undeclared`;
- the table entry left out: the program builds, with only a warning
  that `sys_led_impl` is defined but not used, and runs. `sys_led`
  returns -1 (`p/d (int) $r0` at the `bx lr` of `sys_led`), because the
  number is not in the table, and the LEDs never change. This is the
  omission that gives no error, which is why the warning is worth
  reading.

**The four places, in order:**

```c
/* lib/syscalls.h */
#define SYS_LED 4
int sys_led( uint32_t bits );
```

```asm
/* lib/syscalls.s */
    .global sys_led
    .type sys_led, %function
    .thumb_func
sys_led:
    svc #4
    bx lr
    .size sys_led, . - sys_led
```

```c
/* kernel/syscall.c */
static uint32_t sys_led_impl( uint32_t a0, uint32_t a1, uint32_t a2 )
{
    (void) a1;
    (void) a2;
    FPGAIO->LED = a0 & 0x3;
    return 0;
}
/* ... and in table: */
    [SYS_LED]    = sys_led_impl,
```

`src/app.c` calls `sys_led( ( now / 1000 ) % 2 ? 1 : 2 )` once a second.
The number in the stub is a plain 4 because the assembler cannot read
the C header; the three numbers in use are 1, 2 and 3, so 4 is the next.
In gdb, with a breakpoint on `SysTick_Handler` taken when the uptime is
1500 and again when it is 2500, `p/x FPGAIO->LED` gives `0x1` then
`0x2`: alternate seconds.

With `svc #5` by mistake, the number is past the end of the table, so
`svc_dispatch` stores -1 in the result: `p/d (int) $r0` at the `bx lr`
of `sys_led` gives `-1` where the correct stub gives `0`. The LEDs stay
at `0x3`, the value `main()` wrote, and never change.

### B7. Broken drivers

Each defect is one replacement file in `broken/`. Copy `solution/` to
a working folder, copy the file over its counterpart, `make clean`,
`make run`. The three produce different symptoms, and each was found by
the method below.

**`broken/timer-wrong-ack/timer.c`** replaces `drivers/timer.c`.
*Symptom:* the `timer0:` lines flood the terminal and no `uptime` line
appears. This is the symptom of A3 without an acknowledge, but the
handler does contain one. *Diagnosis:* in gdb, `break *TIMER0_Handler`,
`ignore 1 50` and `continue`: the handler is entered more than 50 times,
and `p/x CMSDK_TIMER0->INTSTATUS` gives `0x1`, the timer's flag is
still set. The acknowledge in the handler writes
`CMSDK_UART0->INTSTATUS = UART_INT_TX`: it clears a flag of the wrong
device, which is a plausible edit when a handler is copied from the UART
driver. The repair is `CMSDK_TIMER0->INTSTATUS = TIMER_INT_PENDING`.

**`broken/app-writes-nvic/app.c`** replaces `src/app.c`. *Symptom:* the
program runs for a second, then prints `BusFault: CFSR=0x8200` and
stops; the watchdog then reports `watchdog: NMI` and the banner prints
again, and the cycle repeats. *Diagnosis:* `break BusFault_Handler` and
`continue`; `p/x SCB->CFSR` gives `0x8200` (a precise bus error with a
valid fault address), and `bt` lists `<signal handler called>` and then
`app_main () at src/app.c`: the instruction that faulted is in the
application. The application is running unprivileged, and the write to
`NVIC->ICER` is in the system control space, which unprivileged code may
not access. A write to a device register such as `FPGAIO->LED` from the
same place does not fault on this board (see B4). The repair is to ask
the kernel, as the other services do, or not to write the register at
all. The fault handler halts, so SysTick no longer runs and the watchdog
of B3 is no longer fed; that is the NMI and the reboot.

**`broken/misspelt-handler/uart.c`** replaces `drivers/uart.c`.
*Symptom:* the system runs and prints `uptime` lines until a key is
typed; then no `key:` line appears and the `uptime` lines stop, followed
by `watchdog: NMI` and a reboot. *Diagnosis:* `break Default_Handler`
and `continue` after typing a key: the breakpoint is hit, and
`p/x $xpsr & 0x1ff` gives `0x10`, exception 16, which is IRQ 0, the
receive interrupt of UART0. The interrupt has been taken, but the
vector table sent it to `Default_Handler`, which loops. The table names
`UART0RX_Handler`; the function in `broken/misspelt-handler/uart.c` is
named `UART0Rx_Handler`. It compiles without complaint, because it is
an ordinary function with an ordinary name, and the weak default in
`boot/startup.s` stays in place. The repair is the spelling.

### B8. (Optional, stretch) Pend an IRQ from software, measure its latency

In a copy. `TIMER1` is left stopped, its priority set to `0x40` and
enabled in `NVIC->ISER`; `main()`, before the drop, repeats five times:

```c
uint32_t before = SysTick->VAL;
NVIC->ISPR[TIMER1_IRQn / 32] = 1UL << ( TIMER1_IRQn % 32 );
kprintf( "latency %u\n", before - entry );
```

and `TIMER1_Handler`'s first line is `entry = SysTick->VAL;`, with
`entry` a `volatile uint32_t`. The handler needs no acknowledge: the
device line is low, so the pending bit clears on entry. SysTick counts
down, hence `before - entry`. One run printed 650, 75, 25, 25, 25
(**this build**; the numbers change from run to run, the pattern does
not). The first is larger because QEMU translates the code the first
time it runs; the rest come in coarse steps because the emulated clock
moves in steps, not per instruction. A real Cortex-M3 takes 12 cycles
from the pend to the first instruction of the handler.
