#include <kernel_atomic.h>

void kernel_begin_critical(kernel_atomic_t volatile *atomic) {
	*atomic = __get_PRIMASK();
	__disable_irq();
	__DMB();
}

void kernel_end_critical(kernel_atomic_t volatile *atomic) {
	__DMB();
	__set_PRIMASK(*atomic);
}
