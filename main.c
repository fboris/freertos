#define USE_STDPERIPH_DRIVER
#include "stm32f4xx_conf.h"
#include "stm32f4_discovery.h"

int main()
{
	init_rs232();
	enable_rs232_interrupts();
	return 0;
}

void vApplicationTickHook()
{}