#define USE_STDPERIPH_DRIVER
#include "stm32f4xx_conf.h"
#include "stm32f4_discovery.h"
void send_byte(uint8_t b)
{
    /* Wait until the RS232 port can receive another byte. */
    //while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
	
    USART_SendData(USART2, b);
    while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
}
void USART2_IRQHandler()
{
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET){

		send_byte(USART_ReceiveData(USART2));

	}
}
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
	init_rs232();
	enable_rs232_interrupts();
	int i = 0;
	while(1)
	{

		for(i=0;i<10000;i++);
	}
	return 0;
}

void vApplicationTickHook()
{}