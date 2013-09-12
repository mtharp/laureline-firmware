#include "common.h"


void __attribute__((naked))
HardFault_Handler(void) {
	__asm volatile (
		"tst    lr, #4  \n"
		"ite    eq      \n"
		"mrseq  r0, msp \n"
		"mrsne  r0, psp \n"
		"b      HardFault_Handler_c\n"
	);
}



static void **psp;
register void *sp asm("sp");
void
HardFault_Handler_c(uint32_t *args) {
	uint32_t r0, r1, r2, r3, r12, lr, pc, psr;
	uint32_t bfar, cfsr, hfsr, dfsr, afsr;
	__asm volatile ("mrs %0, psp" : "=r"(psp) : :);
	r0  = args[0];
	r1  = args[1];
	r2  = args[2];
	r3  = args[3];
	r12 = args[4];
	lr  = args[5];
	pc  = args[6];
	psr = args[7];
	bfar = SCB->BFAR;
	cfsr = SCB->CFSR;
	hfsr = SCB->HFSR;
	dfsr = SCB->DFSR;
	afsr = SCB->AFSR;
	(void)r0; (void)r1; (void)r2; (void)r3;
	(void)r12; (void)lr; (void)pc; (void)psr;
	(void)bfar; (void)cfsr; (void)hfsr;
	(void)dfsr; (void)afsr;

	if (lr & 0x04) {
		/* Was using process stack -- replace the main stack pointer with it so
		 * the debugger can see */
		sp = psp;
	}
	(void)sp;

	while (1) {}
}
