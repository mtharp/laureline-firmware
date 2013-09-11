/*
 * http://koti.kapsi.fi/jpa/stuff/other/stm32-hardfault-backtrace.html
 */

void **HARDFAULT_PSP;
register void *stack_pointer asm("sp");

void
HardFault_Handler(void) {
	// Hijack the process stack pointer to make backtrace work
	asm("mrs %0, psp" : "=r"(HARDFAULT_PSP) : :);
	stack_pointer = HARDFAULT_PSP;
	while(1);
}
