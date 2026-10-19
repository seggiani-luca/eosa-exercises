#ifndef CMSDK_H
#define CMSDK_H

#include <stdint.h>

/* SysTick */

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

/* SCB */

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
	volatile uint32_t HFSR;	   // hard fault status
	volatile uint32_t DFSR;    // debug fault status
	volatile uint32_t MMFAR;   // memory fault address
	volatile uint32_t BFAR;    // bus fault address
	volatile uint32_t AFSR;    // auxilliary fault status
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

#define SCB_CFSR_IACCVIOL (1UL << 0)  // instruction access violation
#define SCB_CFSR_DACCVIOL (1UL << 1)  // data access violation
#define SCB_CFSR_MUNSTKERR (1UL << 3) // unstacking fault 
#define SCB_CFSR_MSTKERR (1UL << 4)   // exception stacking fault 

/* UART */

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

/* NVIC */

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

// IRQ numbers
#define UART0RX_IRQn 0	  // UART0 received a byte
#define UART0TX_IRQn 1	  // UART0 finished sending a byte
#define UART1RX_IRQn 2	  // UART1 received a byte
#define UART1TX_IRQn 3	  // UART1 finished sending a byte
#define UART2RX_IRQn 4	  // UART2 received a byte
#define UART2TX_IRQn 5	  // UART2 finished sending a byte
#define TIMER0_IRQn 8	  // APB timer 0 reached zero
#define TIMER1_IRQn 9	  // APB timer 1 reached zero
#define DUALTIMER_IRQn 10 // dual timer
#define UARTOVF_IRQn 12	  // combined UART overflow

/* MPU */

// MPU typedef
typedef struct {
	volatile uint32_t TYPE; // 0x00 how many regions this MPU has (bits 15:8)
	volatile uint32_t CTRL; // 0x04 control: turn the MPU on or off
	volatile uint32_t RNR; // 0x08 region number: which region RBAR/RASR talk to
	volatile uint32_t RBAR; // 0x0C region base address (and region number)
	volatile uint32_t RASR; // 0x10 region attributes and size
} MPU_TypeDef;

// core MPU
#define MPU ((MPU_TypeDef *)0xE000ED90UL)

// MPU registers
#define MPU_CTRL_ENABLE (1UL << 0)	   // the MPU checks every access
#define MPU_CTRL_PRIVDEFENA (1UL << 2) // privileged code gets default map

#define MPU_RBAR_REGION_MASK (0xFUL) // bits 3:0: region number
#define MPU_RBAR_VALID (1UL << 4)	 // write RBAR with the region number

#define MPU_RASR_ENABLE (1UL << 0)			 // this region is active
#define MPU_RASR_SIZE(n) (((n) - 1UL) << 1)	 // region size = 2^n bytes
#define MPU_RASR_B (1UL << 16)				 // bufferable
#define MPU_RASR_C (1UL << 17)				 // cacheable
#define MPU_RASR_S (1UL << 18)				 // shareable
#define MPU_RASR_AP(v) ((uint32_t)(v) << 24) // access permissions, 3 bits
#define MPU_RASR_XN (1UL << 28) // execute never: no instruction fetch here

#define MPU_AP_NO_NO 0UL // 000: privileged: no; unprivileged: no
#define MPU_AP_RW_NO 1UL // 001: privileged: RW; unprivileged: no
#define MPU_AP_RW_RO 2UL // 010: privileged: RW; unprivileged: RO
#define MPU_AP_RW_RW 3UL // 011: privileged: RW; unprivileged: RW
#define MPU_AP_RO_NO 5UL // 101: privileged: RO; unprivileged: no
#define MPU_AP_RO_RO 6UL // 110: privileged: RO; unprivileged: RO

/* Timer */

// timer typedef
typedef struct {
	volatile uint32_t CTRL;		 // control flags
	volatile uint32_t VALUE;	 // current counter value (counts down)
	volatile uint32_t RELOAD;	 // value loaded when the counter reaches 0
	volatile uint32_t INTSTATUS; // interrupt status
} CMSDK_TIMER_TypeDef;

#define CMSDK_TIMER0 ((CMSDK_TIMER_TypeDef *)0x40000000UL)
#define CMSDK_TIMER1 ((CMSDK_TIMER_TypeDef *)0x40001000UL)

#define TIMER_CTRL_EN (1UL << 0)	 // enable the timer
#define TIMER_CTRL_EXTIN (1UL << 1)	 // use the external input as enable
#define TIMER_CTRL_EXTCLK (1UL << 2) // use the external input as clock
#define TIMER_CTRL_IRQEN (1UL << 3)	 // raise an IRQ on timeout
#define TIMER_INT_PENDING (1UL << 0) // IRQ pending; write 1 to clear

/* Watchdog */

// watchdog typedef
typedef struct {
	volatile uint32_t LOAD;	  // start value of the down counter
	volatile uint32_t VALUE;  // current counter value
	volatile uint32_t CTRL;	  // control flags
	volatile uint32_t INTCLR; // write to clear interrupt and reload
	volatile uint32_t RIS;	  // raw interrupt status
	volatile uint32_t MIS;	  // masked interrupt status
	uint32_t RESERVED[762];	  // gap up to 0xC00
	volatile uint32_t LOCK;	  // write key to unlock, anything else to lock
} CMSDK_WDOG_TypeDef;

#define CMSDK_WDOG ((CMSDK_WDOG_TypeDef *)0x40008000UL)

#define WDOG_UNLOCK_KEY 0x1ACCE551UL // write it to LOCK to unlock
#define WDOG_CTRL_INTEN (1UL << 0)	 // enable the interrupt (NMI)
#define WDOG_CTRL_RESEN (1UL << 1)	 // reset the system on the second expiry

#endif
