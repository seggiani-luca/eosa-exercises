#ifndef CMSDK_H
#define CMSDK_H

#include <stdint.h>

// SysTick typedef
typedef struct {
	volatile uint32_t CTRL;	 // control and status
	volatile uint32_t LOAD;	 // reload value (24 bits)
	volatile uint32_t VAL;	 // current value; any write clears it
	volatile uint32_t CALIB; // calibration value (not used here)
} SysTick_TypeDef;

// board SysTick
#define SysTick ((SysTick_TypeDef *)0xE000E010UL)

// SysTick registers
#define SysTick_CTRL_ENABLE (1UL << 0)	   // start the counter
#define SysTick_CTRL_TICKINT (1UL << 1)	   // raise exception on wrap
#define SysTick_CTRL_CLKSOURCE (1UL << 2)  // core (0) / reference (1)
#define SysTick_CTRL_COUNTFLAG (1UL << 16) // 1 if the counter wrapped

// SCB typedef
typedef struct {
	volatile uint32_t CPUID;   // CPU identification
	volatile uint32_t ICSR;	   // interrupt control and state
	volatile uint32_t VTOR;	   // ISR table offset
	volatile uint32_t AIRCR;   // application interrupt and reset control
	volatile uint32_t SCR;	   // system control
	volatile uint32_t CCR;	   // configuration and control
	volatile uint8_t SHPR[12]; // priority byte of system handlers
	volatile uint32_t SHCSR;   // system handler control and state
	volatile uint32_t CFSR;	   // configurable fault status (Mem/Bus/Usage)
	volatile uint32_t HFSR;	   // HardFault status
} SCB_TypeDef;

// CPU SCB
#define SCB ((SCB_TypeDef *)0xE000ED00UL)

// SCB registers
#define SCB_SHPR_SVCALL 7	// SHPR[7]  priority of SVCall
#define SCB_SHPR_PENDSV 10	// SHPR[10] priority of PendSV
#define SCB_SHPR_SYSTICK 11 // SHPR[11] priority of SysTick

#define SCB_CCR_UNALIGN_TRP (1UL << 3) // fault on unaligned access
#define SCB_CCR_DIV_0_TRP (1UL << 4)   // fault on divide by zero

#define SCB_SHCSR_MEMFAULTENA (1UL << 16) // enable the MemManage fault handler
#define SCB_SHCSR_BUSFAULTENA (1UL << 17) // enable the BusFault handler
#define SCB_SHCSR_USGFAULTENA (1UL << 18) // enable the UsageFault handler

#define SCB_HFSR_VECTTBL (1UL << 1) // fault while reading the vector table
#define SCB_HFSR_FORCED (1UL << 30) // fault escalated to HardFault

#define SCB_CFSR_IBUSERR (1UL << 8)		// BusFault: instruction fetch error
#define SCB_CFSR_PRECISERR (1UL << 9)	// BusFault: precise data access error
#define SCB_CFSR_UNDEFINSTR (1UL << 16) // UsageFault: undefined instruction
#define SCB_CFSR_INVSTATE (1UL << 17)	// UsageFault: bad EPSR state
#define SCB_CFSR_DIVBYZERO (1UL << 25)	// UsageFault: divide by zero

// UART typedef
typedef struct {
	volatile uint32_t DATA;		 // send/receive buffer
	volatile uint32_t STATE;	 // status flags
	volatile uint32_t CTRL;		 // control flags
	volatile uint32_t INTSTATUS; // interrupt status
	volatile uint32_t BAUDDIV;	 // baud rate divider
} CMSDK_UART_TypeDef;

// UART0, UART1, UART2
#define CMSDK_UART0 ((CMSDK_UART_TypeDef *)0x40004000UL)
#define CMSDK_UART1 ((CMSDK_UART_TypeDef *)0x40005000UL)
#define CMSDK_UART2 ((CMSDK_UART_TypeDef *)0x40006000UL)

// UART registers
#define UART_DATA (0xFFul << 0) // data byte

#define UART_STATE_RXOR (0x1ul << 3) // receive overrun
#define UART_STATE_TXOR (0x1ul << 2) // transmit overrun
#define UART_STATE_RXBF (0x1ul << 1) // receive buffer full
#define UART_STATE_TXBF (0x1ul << 0) // transmit buffer full

#define UART_CTRL_HSTM (0x01ul << 6)	  // high-speed test mode
#define UART_CTRL_RXORIRQEN (0x01ul << 5) // enable receive overrun interrupt
#define UART_CTRL_TXORIRQEN (0x01ul << 4) // enable transmit overrun interrupt
#define UART_CTRL_RXIRQEN (0x01ul << 3)	  // enable receive interrupt
#define UART_CTRL_TXIRQEN (0x01ul << 2)	  // enable transmit interrupt
#define UART_CTRL_RXEN (0x01ul << 1)	  // enable receiver
#define UART_CTRL_TXEN (0x01ul << 0)	  // enable transmitter

#define UART_INTSTATUS_RXORIRQ (0x01ul << 3) // receive overrun interrupt
#define UART_INTSTATUS_TXORIRQ (0x01ul << 2) // transmit overrun interrupt
#define UART_INTSTATUS_RXIRQ (0x01ul << 1)	 // receive interrupt
#define UART_INTSTATUS_TXIRQ (0x01ul << 0)	 // transmit interrupt

#define UART_BAUDDIV (0xFFFFFul << 0) // baud rate divider

// NVIC typedef
typedef struct {
	volatile uint32_t ISER[8]; // enable IRQ
	uint32_t RESERVED0[24];
	volatile uint32_t ICER[8]; // disable IRQ
	uint32_t RESERVED1[24];
	volatile uint32_t ISPR[8]; // set pending IRQ
	uint32_t RESERVED2[24];
	volatile uint32_t ICPR[8]; // clear pending IRQ
	uint32_t RESERVED3[24];
	volatile uint32_t IABR[8]; // is IRQ active?
	uint32_t RESERVED4[56];
	volatile uint8_t IPR[240]; // priority byte per-IRQ
} NVIC_TypeDef;

// core NVIC
#define NVIC ((NVIC_TypeDef *)0xE000E100UL)

#endif
