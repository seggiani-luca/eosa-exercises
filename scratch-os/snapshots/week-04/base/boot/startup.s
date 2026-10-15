/*
 * Embedded Operating Systems and Architecture course
 * Licensed under the MIT License. See LICENSE in the repository root.
 *
 * Contact: Stefano Di Carlo <stefano.dicarlo@polito.it>
 *
 * Boot code for the Cortex-M3, in assembly: the vector table, the reset
 * handler (which builds the C runtime by hand and calls main), and the
 * default exception handler.
 */

/* ------------------------------------------------------------------ */
/* Assembler settings                                                  */
/* ------------------------------------------------------------------ */

    .syntax unified             /* modern ARM assembly syntax */
    .cpu cortex-m3              /* only instructions this CPU really has */
    .thumb                      /* Thumb code (the only kind Cortex-M runs) */

/* ------------------------------------------------------------------ */
/* Symbols defined elsewhere (by scripts/mps2_m3.ld): addresses only.  */
/* ------------------------------------------------------------------ */

/* _estack, _sidata, _sdata, _edata, _sbss, _ebss: linker-script addresses.
 * main is defined in src/main.c. */

/* ================================================================== */
/* 1. The vector table                                                 */
/* ================================================================== */

/* An array of 32-bit words at address 0: word N is the handler for
 * exception N. The CPU reads it on reset and on every exception. */

/* `.section NAME,"a",%progbits`: put what follows into .isr_vector. */
    .section .isr_vector,"a",%progbits
    .global isr_vector          /* visible to other files */
    .type isr_vector, %object   /* it is data */

isr_vector:
    .word _estack               /* 0: initial SP = top of RAM */
    .word Reset_Handler         /* 1: reset: first code after power-on */
    .word NMI_Handler           /* 2: NMI (non-maskable interrupt) */
    .word HardFault_Handler     /* 3: HardFault (any fault with nowhere else to go) */
    .word MemManage_Handler     /* 4: MemManage (memory protection violation) */
    .word BusFault_Handler      /* 5: BusFault (bad memory access on the bus) */
    .word UsageFault_Handler    /* 6: UsageFault (bad instruction, divide by 0, ...) */
    .word 0                     /* 7: reserved, must be 0 */
    .word 0                     /* 8: reserved */
    .word 0                     /* 9: reserved */
    .word 0                     /* 10: reserved */
    .word SVC_Handler           /* 11: SVCall (the `svc` instruction) */
    .word DebugMon_Handler      /* 12: debug monitor */
    .word 0                     /* 13: reserved */
    .word PendSV_Handler        /* 14: PendSV (a software-requested exception) */
    .word SysTick_Handler       /* 15: SysTick (the CPU's built-in timer) */

/* Device interrupts: IRQ n is exception 16 + n (word 16 + n). Which
 * device is on which IRQ is set by the board (MPS2-AN385). */
    .word UART0RX_Handler       /* 16 = IRQ 0: UART0 received a byte */
    .word UART0TX_Handler       /* 17 = IRQ 1: UART0 finished sending a byte */
    .word UART1RX_Handler       /* 18 = IRQ 2: UART1 received a byte */
    .word UART1TX_Handler       /* 19 = IRQ 3: UART1 finished sending a byte */
    .word UART2RX_Handler       /* 20 = IRQ 4: UART2 received a byte */
    .word UART2TX_Handler       /* 21 = IRQ 5: UART2 finished sending a byte */
    .word Default_Handler       /* 22 = IRQ 6: GPIO0 combined (not used in this course) */
    .word Default_Handler       /* 23 = IRQ 7: GPIO1 combined (not used in this course) */
    .word TIMER0_Handler        /* 24 = IRQ 8: timer 0 reached zero */
    .word TIMER1_Handler        /* 25 = IRQ 9: timer 1 reached zero */
    .word DUALTIMER_Handler     /* 26 = IRQ 10: dual timer */
    .word Default_Handler       /* 27 = IRQ 11: SPI (not used in this course) */
    .word UARTOVF_Handler       /* 28 = IRQ 12: some UART lost a byte (overflow) */
    .word Default_Handler       /* 29 = IRQ 13: Ethernet (not used in this course) */
    .word Default_Handler       /* 30 = IRQ 14: audio I2S (not used in this course) */
    .word Default_Handler       /* 31 = IRQ 15: touchscreen (not used in this course) */
    .word Default_Handler       /* 32 = IRQ 16: other board interrupt (not used in this course) */
    .word Default_Handler       /* 33 = IRQ 17: other board interrupt (not used in this course) */
    .word Default_Handler       /* 34 = IRQ 18: other board interrupt (not used in this course) */
    .word Default_Handler       /* 35 = IRQ 19: other board interrupt (not used in this course) */
    .word Default_Handler       /* 36 = IRQ 20: other board interrupt (not used in this course) */
    .word Default_Handler       /* 37 = IRQ 21: other board interrupt (not used in this course) */
    .word Default_Handler       /* 38 = IRQ 22: other board interrupt (not used in this course) */
    .word Default_Handler       /* 39 = IRQ 23: other board interrupt (not used in this course) */
    .word Default_Handler       /* 40 = IRQ 24: other board interrupt (not used in this course) */
    .word Default_Handler       /* 41 = IRQ 25: other board interrupt (not used in this course) */
    .word Default_Handler       /* 42 = IRQ 26: other board interrupt (not used in this course) */
    .word Default_Handler       /* 43 = IRQ 27: other board interrupt (not used in this course) */
    .word Default_Handler       /* 44 = IRQ 28: other board interrupt (not used in this course) */
    .word Default_Handler       /* 45 = IRQ 29: other board interrupt (not used in this course) */
    .word Default_Handler       /* 46 = IRQ 30: other board interrupt (not used in this course) */
    .word Default_Handler       /* 47 = IRQ 31: other board interrupt (not used in this course) */

    .size isr_vector, . - isr_vector /* record its size in bytes */

/* ------------------------------------------------------------------ */
/* Default handler names                                               */
/* ------------------------------------------------------------------ */

/* Each handler defaults to Default_Handler. `.weak` lets a C file with
 * the same name (e.g. void SysTick_Handler(void)) replace it. */
    .weak NMI_Handler           /* NMI: may be replaced from C */
    .thumb_set NMI_Handler, Default_Handler /* otherwise it is Default_Handler */

    .weak HardFault_Handler     /* HardFault: may be replaced from C */
    .thumb_set HardFault_Handler, Default_Handler /* otherwise it is Default_Handler */

    .weak MemManage_Handler     /* MemManage: may be replaced from C */
    .thumb_set MemManage_Handler, Default_Handler /* otherwise it is Default_Handler */

    .weak BusFault_Handler      /* BusFault: may be replaced from C */
    .thumb_set BusFault_Handler, Default_Handler /* otherwise it is Default_Handler */

    .weak UsageFault_Handler    /* UsageFault: may be replaced from C */
    .thumb_set UsageFault_Handler, Default_Handler /* otherwise it is Default_Handler */

    .weak SVC_Handler           /* SVCall: may be replaced (C or assembly) */
    .thumb_set SVC_Handler, Default_Handler /* otherwise it is Default_Handler */

    .weak DebugMon_Handler      /* debug monitor: may be replaced from C */
    .thumb_set DebugMon_Handler, Default_Handler /* otherwise it is Default_Handler */

    .weak PendSV_Handler        /* PendSV: may be replaced from C */
    .thumb_set PendSV_Handler, Default_Handler /* otherwise it is Default_Handler */

    .weak SysTick_Handler       /* SysTick: may be replaced from C */
    .thumb_set SysTick_Handler, Default_Handler /* otherwise it is Default_Handler */

    .weak UART0RX_Handler       /* UART0 receive: may be replaced from C */
    .thumb_set UART0RX_Handler, Default_Handler /* otherwise it is Default_Handler */

    .weak UART0TX_Handler       /* UART0 transmit: may be replaced from C */
    .thumb_set UART0TX_Handler, Default_Handler /* otherwise it is Default_Handler */

    .weak UART1RX_Handler       /* UART1 receive: may be replaced from C */
    .thumb_set UART1RX_Handler, Default_Handler /* otherwise it is Default_Handler */

    .weak UART1TX_Handler       /* UART1 transmit: may be replaced from C */
    .thumb_set UART1TX_Handler, Default_Handler /* otherwise it is Default_Handler */

    .weak UART2RX_Handler       /* UART2 receive: may be replaced from C */
    .thumb_set UART2RX_Handler, Default_Handler /* otherwise it is Default_Handler */

    .weak UART2TX_Handler       /* UART2 transmit: may be replaced from C */
    .thumb_set UART2TX_Handler, Default_Handler /* otherwise it is Default_Handler */

    .weak TIMER0_Handler        /* timer 0: may be replaced from C */
    .thumb_set TIMER0_Handler, Default_Handler /* otherwise it is Default_Handler */

    .weak TIMER1_Handler        /* timer 1: may be replaced from C */
    .thumb_set TIMER1_Handler, Default_Handler /* otherwise it is Default_Handler */

    .weak DUALTIMER_Handler     /* dual timer: may be replaced from C */
    .thumb_set DUALTIMER_Handler, Default_Handler /* otherwise it is Default_Handler */

    .weak UARTOVF_Handler       /* UART overflow: may be replaced from C */
    .thumb_set UARTOVF_Handler, Default_Handler /* otherwise it is Default_Handler */

/* ================================================================== */
/* 2. Reset_Handler                                                    */
/* ================================================================== */

    .section .text,"ax",%progbits   /* code goes into the normal code section */
    .global Reset_Handler       /* the linker script names it as ENTRY */
    .type Reset_Handler, %function /* it is code */
    .thumb_func                 /* Thumb code (bit 0 set in its address) */
Reset_Handler:

    /* Step 1: mask interrupts while RAM and the registers are half set up. */
    cpsid i                     /* disable interrupts: none can run from now on */

    /* Step 2: set the stack pointer. */
    ldr r0, =_estack            /* r0 = address of the top of the stack */
    mov sp, r0                  /* SP = r0: the stack now starts there */

    /* Step 3: copy .data initial values from FLASH (_sidata) to RAM (_sdata.._edata). */
    ldr r0, =_sidata            /* src = &_sidata (r0 is src) */
    ldr r1, =_sdata             /* dst = &_sdata  (r1 is dst) */
    ldr r2, =_edata             /* r2 = &_edata, the end of the loop */
copy_data:
    cmp r1, r2                  /* compare dst with &_edata ... */
    bhs copy_data_done          /* ... if dst >= &_edata, the loop is over */
    ldr r3, [r0], #4            /* r3 = *src; src++ */
    str r3, [r1], #4            /* *dst = r3; dst++ */
    b copy_data                 /* go back and test the while condition */
copy_data_done:

    /* Step 4: zero .bss (_sbss.._ebss). */
    ldr r1, =_sbss              /* dst = &_sbss   (r1 is dst) */
    ldr r2, =_ebss              /* r2 = &_ebss, the end of the loop */
    movs r3, #0                 /* r3 = 0, the value to write */
zero_bss:
    cmp r1, r2                  /* compare dst with &_ebss ... */
    bhs zero_bss_done           /* ... if dst >= &_ebss, the loop is over */
    str r3, [r1], #4            /* *dst = 0; dst++ */
    b zero_bss                  /* go back and test the while condition */
zero_bss_done:

    /* Step 5: point VTOR (0xE000ED08) at the vector table. */
    ldr r0, =0xE000ED08         /* r0 = address of VTOR */
    ldr r1, =isr_vector         /* r1 = (uint32_t)isr_vector */
    str r1, [r0]                /* VTOR = r1 */

    /* Step 6: enable the MemManage, BusFault and UsageFault handlers (SHCSR). */
    ldr r0, =0xE000ED24         /* r0 = address of SHCSR */
    ldr r1, [r0]                /* r1 = SCB_SHCSR (read) */
    orr r1, r1, #0x00070000     /* r1 |= (1u<<16)|(1u<<17)|(1u<<18) (modify) */
    str r1, [r0]                /* SCB_SHCSR = r1 (write) */

    /* Step 7: make integer division by zero raise a UsageFault (CCR bit 4). */
    ldr r0, =0xE000ED14         /* r0 = address of CCR */
    ldr r1, [r0]                /* r1 = SCB_CCR (read) */
    orr r1, r1, #(1 << 4)       /* r1 |= (1u << 4), DIV_0_TRP (modify) */
    str r1, [r0]                /* SCB_CCR = r1 (write) */

    /* Step 8: set exception priorities (lower number = more urgent). */
    ldr r0, =0xE000ED18         /* prio = r0 = address of SHPR1 byte 0 */
    movs r1, #0x00              /* r1 = 0x00, most urgent */
    strb r1, [r0, #0]           /* prio[0] = 0x00: MemManage */
    strb r1, [r0, #1]           /* prio[1] = 0x00: BusFault */
    strb r1, [r0, #2]           /* prio[2] = 0x00: UsageFault */
    movs r1, #0x80              /* r1 = 0x80, middle priority */
    strb r1, [r0, #7]           /* prio[7] = 0x80: SVCall */
    movs r1, #0xE0              /* r1 = 0xE0, least urgent */
    strb r1, [r0, #10]          /* prio[10] = 0xE0: PendSV */
    strb r1, [r0, #11]          /* prio[11] = 0xE0: SysTick */

    /* Step 9: setup is complete: unmask interrupts. */
    cpsie i                     /* enable interrupts again: PRIMASK = 0 */

    /* Step 10: call main(); if it ever returns, stop here. */
    bl main                     /* main(); "branch with link" saves the return address in LR */
hang:
    b hang                      /* for (;;) {}: main should never return; if it does, stay here */

/* Literal pool: `ldr rX, =VALUE` stores VALUE in this table, placed
 * after the code where the CPU never runs it as an instruction. */
    .ltorg

    .size Reset_Handler, . - Reset_Handler /* record its size in bytes */

/* ================================================================== */
/* 3. Default_Handler                                                  */
/* ================================================================== */

/* Every exception without its own handler lands here. It is a bug, so
 * spin forever: the CPU freezes at a known place.
 * C equivalent:
 *     void Default_Handler(void) {
 *         for (;;) { }
 *     }
 */
    .global Default_Handler     /* visible to the linker and the debugger */
    .type Default_Handler, %function /* it is code */
    .thumb_func                 /* Thumb code (bit 0 set in its address) */
Default_Handler:
    b Default_Handler           /* for (;;) {}: jump to itself forever */
    .size Default_Handler, . - Default_Handler /* record its size in bytes */
