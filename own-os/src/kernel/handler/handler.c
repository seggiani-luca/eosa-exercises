#include "../include/cmsdk.h"
#include "../include/log.h"

void handler_init() {
	// (unrelated) allow devices over SVC
	SCB->SHPR[7] = 0xE0;

	// enable all fault types
	SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA
	           |  SCB_SHCSR_BUSFAULTENA
	           |  SCB_SHCSR_USGFAULTENA; 
}

void NMI_Handler() {
	kprintf("NMI fault\n");

    uint32_t status = CMSDK_WDOG->RIS;
	if(status) {
		kprintf("watchdog fired \n");
		kprintf("reset will follow if not handled \n");
	}

	while (1)
		;
}

void HardFault_Handler() {
	kprintf("HardFault\n");
	kprintf("HFSR: %x\n", SCB->HFSR);
	kprintf("CFSR: %x\n", SCB->CFSR);

	if (SCB->HFSR & SCB_HFSR_VECTTBL)
		kprintf("vector table read fault\n");

	if (SCB->HFSR & SCB_HFSR_FORCED)
		kprintf("forced configurable fault\n");

	while (1)
		;
}

void Memory_Handler() {
	kprintf("MemManage fault\n");
	kprintf("MMFAR: %x\n", SCB->MMFAR);
	kprintf("CFSR: %x\n", SCB->CFSR);

	if(SCB->CFSR & SCB_CFSR_IACCVIOL)
		kprintf("instruction access violation\n");
	
	if(SCB->CFSR & SCB_CFSR_DACCVIOL)
		kprintf("data access violation\n");
	
	if(SCB->CFSR & SCB_CFSR_MUNSTKERR)
		kprintf("fault while unstacking\n");
	
	if(SCB->CFSR & SCB_CFSR_MSTKERR)
		kprintf("fault while stacking (exception)\n");

	while (1)
		;
}

void Bus_Handler() {
	uint32_t cfsr = SCB->CFSR;

	kprintf("BusFault\n");
	kprintf("CFSR: %x\n", cfsr);

	if (cfsr & SCB_CFSR_IBUSERR)
		kprintf("instruction bus error\n");

	if (cfsr & SCB_CFSR_PRECISERR)
		kprintf("precise data access error\n");

	while (1)
		;
}

void Usage_Handler() {
	uint32_t cfsr = SCB->CFSR;

	kprintf("UsageFault\n");
	kprintf("CFSR: %x\n", cfsr);

	if (cfsr & SCB_CFSR_UNDEFINSTR)
		kprintf("undefined instruction\n");

	if (cfsr & SCB_CFSR_INVSTATE)
		kprintf("invalid state\n");

	if (cfsr & SCB_CFSR_DIVBYZERO)
		kprintf("division by zero\n");

	while (1)
		;
}

void Default_Handler() {
	kprintf("Unhandled exception\n");
	while (1)
		;
}
