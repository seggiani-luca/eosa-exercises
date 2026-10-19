# Week 4 laboratory — hints

Hints for the exercises of [`README.md`](README.md), to be read when an
exercise does not yield to your own attempt and before its worked
solution in [`SOLUTIONS.md`](SOLUTIONS.md). Each exercise has three
levels. Open one at a time, and stop at the first that lets you
continue: every level is complete in itself, and none contains the
finished code.

1. **The concept** — what to think about, without naming a register.
2. **The rule** — the register, bit or rule involved.
3. **The shape** — the order of the steps, as a skeleton to fill in.

An exercise without a section here (A1, B8) has no step that withholds
anything.

---

## Part A

### A2. Interrupt-driven UART receive

<details><summary>Level 1 — the concept</summary>

Follow one received byte from the wire to your code. List every stage
that must agree before the processor stops what it is doing and enters
your handler, and for each say where in the program it is configured.
Then ask the opposite question about the end of the path: what tells the
hardware that the handler has finished with the event?

When the program does not behave as you expect, find out how far along
the path the event gets before you change any code. The debugger can
show whether the handler is ever entered.
</details>

<details><summary>Level 2 — the rule</summary>

There are two enables on the path. The device has one in `CTRL`
(`UART_CTRL_RXIRQEN`), and the NVIC has one per interrupt in
`NVIC->ISER`. A priority byte in `NVIC->IPR` belongs to the second.
Without the NVIC bit the device raises the line, `NVIC->ISPR` shows it
pending, and nothing runs.

The NVIC lines of this board are level-sensitive. The line stays active
until the device's own flag is cleared: write the bit to `INTSTATUS`
(write 1 to clear). Without it the handler is entered again as soon as
it returns.

The handler writes the head index of the ring buffer and the reader
writes the tail index. The reader's update of the tail must not be
interrupted by the handler; `irq_save` and `irq_restore` are in
`kernel/irq.h`.
</details>

<details><summary>Level 3 — the shape</summary>

```
uart_init:
    <enable the receive interrupt in the device, keeping TX and RX on>
    <set the priority of UART0RX_IRQn>
    <enable UART0RX_IRQn in the NVIC>

UART0RX_Handler:
    c    = <the byte read from the device>
    next = <rx_head + 1, wrapped to the buffer size>
    if <the buffer is not full>:
        <store c at rx_head>
        <advance rx_head>
    <acknowledge the device>

uart_getc:
    s = <mask interrupts>
    c = -1
    if <the buffer is not empty>:
        c = <the byte at rx_tail>
        <advance rx_tail, wrapped>
    <restore the mask from s>
    return c
```

The transmit side of `drivers/uart.c` has the same ring-buffer
vocabulary, with the roles of head and tail exchanged.
</details>

### A3. A timer driver

<details><summary>Level 1 — the concept</summary>

Three questions fix the driver. How is a duration in milliseconds turned
into the number the timer counts down from, given the clock it counts?
What makes the timer signal, and how is that signal carried to the
processor? What must be true when the handler returns so that the timer
can signal again?
</details>

<details><summary>Level 2 — the rule</summary>

The timer counts down at the processor clock and reloads `RELOAD` at
zero; `CTRL` has an enable bit and an interrupt-enable bit
(`TIMER_CTRL_EN`, `TIMER_CTRL_IRQEN`). The NVIC needs the priority byte
and the enable bit of `TIMER0_IRQn`.

The handler must clear the timer's own flag: write
`TIMER_INT_PENDING` to `INTSTATUS` (write 1 to clear). With the flag
left set, the line stays active and the handler is entered again at
once. The tick count changes in the handler and is read elsewhere, so it
is `volatile`.
</details>

<details><summary>Level 3 — the shape</summary>

```
timer0_init(period_ms):
    RELOAD = <cycles per millisecond> * period_ms
    CTRL   = <enable the counter and its interrupt>
    <set the priority of TIMER0_IRQn>
    <enable TIMER0_IRQn in the NVIC>

TIMER0_Handler:
    <acknowledge the device>
    ticks = ticks + 1
    if <ticks is a multiple of 10>:
        <print the tick count with kprintf>
```

`main()` calls `timer0_init(100)` before the privilege drop.
</details>

### A4. Preemption

<details><summary>Level 1 — the concept</summary>

For one handler to interrupt another, the processor must prefer it. On
this processor the preference is a number, and the smaller number wins.
Decide which of the two timers must have the smaller number, and which
of the other handlers in the system sit between them.
</details>

<details><summary>Level 2 — the rule</summary>

The priorities in force are SVCall `0x80`, UART `0xC0` and SysTick
`0xE0`. A handler is preempted only by an exception whose priority byte
is strictly smaller. The low 9 bits of `xPSR` are the number of the
active exception (16 plus the IRQ number for a device). `EXC_RETURN`,
the value of `lr` on entry, is `0xfffffff9` when the exception
preempted thread mode and `0xfffffff1` when it preempted another
handler.
</details>

<details><summary>Level 3 — the shape</summary>

```
TIMER0 (fast):  priority <smaller than everything else>
                handler prints "T0", or "  T0 (inside T1)" if in_slow
TIMER1 (slow):  priority 0xE0, period 1 s
                handler: acknowledge the device, set in_slow, print
                         "T1 start", run a busy loop of a volatile
                         counter, print "T1 end", clear in_slow
gdb:            break *TIMER1_Handler;  p/x $xpsr & 0x1ff;  p/x $lr
                then the same at *TIMER0_Handler, then bt
```

The loop count that gives a handler of about 300 ms depends on the
machine: adjust it until the three or so `T0` lines fit inside.
</details>

---

## Part B

### B1. A race, and its fix

<details><summary>Level 1 — the concept</summary>

`counter = counter + 1` is not one operation. Write the instructions it
becomes and mark every point between them at which an interrupt could be
taken. For each point, say what the handler's own increment does to the
value the interrupted code holds in a register.
</details>

<details><summary>Level 2 — the rule</summary>

The repair is to make the three steps indivisible with respect to the
handler by masking interrupts around them. `irq_save` masks and returns
the previous state of the mask, and `irq_restore` puts that state back;
this matters when the code may already run with interrupts masked. The
mask instruction has no effect in unprivileged code, so the experiment
belongs in `main()` before the drop.
</details>

<details><summary>Level 3 — the shape</summary>

```
repeat 100000 times:
    s   = <irq_save>
    tmp = counter
    <a short volatile delay loop>
    counter = tmp + 1
    <irq_restore s>
print counter, 100000 + isr_hits, and their difference
```

Run it first without the two mask lines and read the difference.
</details>

### B2. One driver, two UARTs

<details><summary>Level 1 — the concept</summary>

The two UARTs differ only in where their registers are. Decide which
functions take that difference as a parameter and which can keep working
unchanged for the callers that exist already.
</details>

<details><summary>Level 2 — the rule</summary>

The registers of `CMSDK_UART0` and `CMSDK_UART1` have the same layout, so
a pointer to `CMSDK_UART_TypeDef` selects the device. QEMU connects its
serial options to the UARTs in order: the first `-serial` is UART0.
</details>

<details><summary>Level 3 — the shape</summary>

```
uart_putc_polled_on(u, c):  <the body of uart_putc_polled, using u>
uart_putc_polled(c):        uart_putc_polled_on(CMSDK_UART0, c)
uart_init_on(u):            <BAUDDIV and CTRL of u>
main():                     <init UART1; send each byte of the greeting>
run with:                   a second -serial option naming a file
```
</details>

### B3. The watchdog

<details><summary>Level 1 — the concept</summary>

A watchdog reports, and later resets, a system that has stopped doing
something it should do regularly. Decide what the regular action is
here, who performs it, and what the system must still be able to do for
the report itself to be delivered.
</details>

<details><summary>Level 2 — the rule</summary>

The watchdog has no IRQ slot of its own: its interrupt is the
non-maskable one, `NMI_Handler`. Its registers ignore writes while
locked: `WDOG_UNLOCK_KEY` written to `LOCK` opens them, any other value
closes them. The first time the counter reaches zero it raises the NMI
and reloads; the second time, with `WDOG_CTRL_RESEN` set, it resets the
board. Writing `INTCLR` restarts the count.
</details>

<details><summary>Level 3 — the shape</summary>

```
NMI_Handler:       print "watchdog: NMI"
main():            unlock, set LOAD to one second of cycles, set
                   CTRL to the interrupt and reset enables, lock
SysTick_Handler:   unlock, write any value to INTCLR, lock
test:              put an endless loop in SysTick_Handler after a while
```
</details>

### B4. Board LEDs

<details><summary>Level 1 — the concept</summary>

A register that controls hardware can be read and written from the
program, and from the debugger too. Decide what you can learn by reading
it back, and what a read-back that differs from what was written tells
you about the register.
</details>

<details><summary>Level 2 — the rule</summary>

`FPGAIO->LED` has one bit per LED and the board has two. The debugger
reads memory with `x/wx` and knows `FPGAIO` by name because the build
keeps macro information. A debugger read of a peripheral register can
fail while the processor is stopped in unprivileged code; stop inside a
handler first.
</details>

<details><summary>Level 3 — the shape</summary>

```
main():   FPGAIO->LED = <both bits>
gdb:      break *SysTick_Handler;  continue
          x/wx <address of LED>;   p/x FPGAIO->LED
          set var FPGAIO->LED = 0xff;  p/x FPGAIO->LED
```
</details>

### B5. GPIO seen from the bus

<details><summary>Level 1 — the concept</summary>

The emulator logs each access to a device it does not model. Each log
line carries an offset, a direction and sometimes a value. Decode each
line against the register table, then ask which C statement produced
that exact sequence of accesses.
</details>

<details><summary>Level 2 — the rule</summary>

A `|=` on a register is a read followed by a write. A write to a
set/clear register is a single write. An unmodelled device accepts the
write and returns 0 for every read, so a read-modify-write there starts
from 0.
</details>

<details><summary>Level 3 — the shape</summary>

```
gpio_set_output(pin):   OUTENABLESET = <bit for pin>
gpio_write(pin, level): if level: DATAOUT = DATAOUT | <bit>
                        else:     DATAOUT = DATAOUT & ~<bit>
run:                    make run-trace; match each log line to a statement
```
</details>

### B6. Add a system call

<details><summary>Level 1 — the concept</summary>

A system call crosses from the application into the kernel. Trace one
existing call, for example `sys_uptime`, from the application's call to
the kernel code that does the work, and write down each file the call
passes through. A new service must be present at each of them.
</details>

<details><summary>Level 2 — the rule</summary>

Four places: the number and prototype in `lib/syscalls.h`; the assembly
stub with `svc #n` in `lib/syscalls.s`, where the number is written out
because the assembler cannot read the C header; the entry in the
dispatch `table` in `kernel/syscall.c`; and the function that does the
work. Omitting each gives a different message, and omitting the table
entry gives no error at build time.
</details>

<details><summary>Level 3 — the shape</summary>

```
syscalls.h:   SYS_LED = <next free number>, and the prototype of sys_led
syscalls.s:   sys_led: <the stub of another call, with svc #<the same number>>
syscall.c:    sys_led_impl: <write the low two bits of the argument to the LED
              register> and return 0
              table: <an entry for SYS_LED>
app.c:        sys_led(1) and sys_led(2) on alternate seconds
```
</details>

### B7. Broken drivers

<details><summary>Level 1 — the concept</summary>

Before changing anything, describe the symptom precisely: what is the
last thing printed, what does the processor appear to be doing, and
what runs next. Then decide what the debugger can tell you that the
output cannot: which exception is active, which function the processor
is in, and what called it.
</details>

<details><summary>Level 2 — the rule</summary>

Useful in all three: `p/x $xpsr & 0x1ff` for the active exception
(16 plus the IRQ number), `bt` for the call chain, `info symbol` for the
name of an address, and `info breakpoints` for how often a breakpoint
was hit. For a fault, `p/x SCB->CFSR` says what kind. Compare what the
handler does with what the exercise A3 required of a handler. Compare
the name of a handler with the name the vector table uses for the same
interrupt.
</details>

<details><summary>Level 3 — the shape</summary>

```
storm:        break the handler, ignore 50 hits, continue; read INTSTATUS
              of the device the handler is meant to serve
fault:        break the fault handler, continue; bt; the first frame in
              your own code is the instruction that caused it
silent key:   stop after the key; which function is the processor in, and
              which exception number is active?
```
</details>
