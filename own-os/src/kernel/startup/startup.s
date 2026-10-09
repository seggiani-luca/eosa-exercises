.syntax unified

.extern _estack
.extern _sidata, _sdata, _edata
.extern _sbss, _ebss

.extern kernel_main

// Reset_Handler defined below
.extern NMI_Handler
.extern HardFault_Handler
.extern Memory_Handler
.extern Bus_Handler
.extern Usage_Handler
.extern Default_Handler
.extern SysTick_Handler

.extern UART0RX_Handler
.extern UART0TX_Handler
.extern UART1RX_Handler
.extern UART1TX_Handler
.extern UART2RX_Handler
.extern UART2TX_Handler
.extern TIMER0_Handler
.extern TIMER1_Handler
.extern DUALTIMER_Handler
.extern UARTOVF_Handler

.section .isr_vector, "a", %progbits
    // internal
    .word _estack           // initial SP 
    .word Reset_Handler     // reset
    .word NMI_Handler       // NMI
    .word HardFault_Handler // hard fault
    .word Memory_Handler    // memory management 
    .word Bus_Handler       // bus fault
    .word Usage_Handler     // usage fault
    .word 0                 // reserved
    .word 0
    .word 0
    .word 0
    .word SVC_Handler       // supervisor call
    .word Default_Handler   // debug monitor
    .word 0                 // reserved
    .word Default_Handler   // pending supervisor
    .word SysTick_Handler   // SysTick

    // external
    .word UART0RX_Handler   // IRQ 0:  UART0 RX 
    .word UART0TX_Handler   // IRQ 1:  UART0 TX 
    .word UART1RX_Handler   // IRQ 2:  UART1 RX 
    .word UART1TX_Handler   // IRQ 3:  UART1 TX 
    .word UART2RX_Handler   // IRQ 4:  UART2 RX
    .word UART2TX_Handler   // IRQ 5:  UART2 TX 
    .word Default_Handler   // IRQ 6:  GPIO0
    .word Default_Handler   // IRQ 7:  GPIO1
    .word TIMER0_Handler    // IRQ 8:  timer 0
    .word TIMER1_Handler    // IRQ 9:  timer 1
    .word DUALTIMER_Handler // IRQ 10: dual timer
    .word Default_Handler   // IRQ 11: SPI
    .word UARTOVF_Handler   // IRQ 12: UART overflow
    .word Default_Handler   // IRQ 13: Ethernet
    .word Default_Handler   // IRQ 14: audio I2S
    .word Default_Handler   // IRQ 15: touchscreen
    .word Default_Handler   // IRQ 16: board GPIO pins
    .word Default_Handler 
    .word Default_Handler 
    .word Default_Handler 
    .word Default_Handler 
    .word Default_Handler 
    .word Default_Handler 
    .word Default_Handler 
    .word Default_Handler 
    .word Default_Handler 
    .word Default_Handler 
    .word Default_Handler 
    .word Default_Handler 
    .word Default_Handler 
    .word Default_Handler 
    .word Default_Handler 

.weak UART0RX_Handler
.thumb_set UART0RX_Handler, Default_Handler

.weak UART0TX_Handler
.thumb_set UART0TX_Handler, Default_Handler

.weak UART1RX_Handler
.thumb_set UART1RX_Handler, Default_Handler

.weak UART1TX_Handler
.thumb_set UART1TX_Handler, Default_Handler

.weak UART2RX_Handler
.thumb_set UART2RX_Handler, Default_Handler

.weak UART2TX_Handler
.thumb_set UART2TX_Handler, Default_Handler

.weak TIMER0_Handler
.thumb_set TIMER0_Handler, Default_Handler

.weak TIMER1_Handler
.thumb_set TIMER1_Handler, Default_Handler

.weak DUALTIMER_Handler
.thumb_set DUALTIMER_Handler, Default_Handler

.weak UARTOVF_Handler
.thumb_set UARTOVF_Handler, Default_Handler

.section .text    

.thumb_func
.global Reset_Handler
Reset_Handler:

Data_Init:
    ldr r0, =_sdata
    ldr r1, =_edata
    ldr r2, =_sidata

Data_Init_Loop:
    cmp r0, r1
    beq Bss_Init 

    ldr r3, [r2], #4
    str r3, [r0], #4

    b Data_Init_Loop

Bss_Init:
    ldr r0, =_sbss
    ldr r1, =_ebss
    mov r2, #0

Bss_Init_Loop:
    cmp r0, r1 
    beq Main_Init

    str r2, [r0], #4

    b Bss_Init_Loop

Main_Init:
    bl kernel_main
    b .
