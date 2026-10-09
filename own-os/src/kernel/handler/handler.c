#include "../include/cmsdk.h"
#include "../include/serial.h"

void NMI_Handler() {
	kprintf("NMI fault\n");
	while (1)
		;
}

void HardFault_Handler() {
	kprintf("HardFault\n");
	kprintf("HFSR: 0x%08x\n", SCB->HFSR);
	kprintf("CFSR: 0x%08x\n", SCB->CFSR);

	if (SCB->HFSR & SCB_HFSR_VECTTBL)
		kprintf("vector table read fault\n");

	if (SCB->HFSR & SCB_HFSR_FORCED)
		kprintf("forced configurable fault\n");

	while (1)
		;
}

void Memory_Handler() {
	kprintf("MemManage fault\n");
	kprintf("CFSR: 0x%08x\n", SCB->CFSR);
	while (1)
		;
}

void Bus_Handler() {
	uint32_t cfsr = SCB->CFSR;

	kprintf("BusFault\n");
	kprintf("CFSR: 0x%08x\n", cfsr);

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
	kprintf("CFSR: 0x%08x\n", cfsr);

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
