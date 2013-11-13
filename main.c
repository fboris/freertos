#define USE_STDPERIPH_DRIVER
#include "stm32f4xx_conf.h"
#include "stm32f4_discovery.h"
int main()
{
	/*test Led*/
	STM_EVAL_PBInit(BUTTON_USER, BUTTON_MODE_EXTI); 
	STM_EVAL_LEDInit(LED3);
	STM_EVAL_LEDInit(LED4);
	STM_EVAL_LEDInit(LED5);
	STM_EVAL_LEDInit(LED6);
	STM_EVAL_LEDOn(LED3);
	STM_EVAL_LEDOn(LED4);
	STM_EVAL_LEDOn(LED5);
	STM_EVAL_LEDOn(LED6);
	return 0;
}

