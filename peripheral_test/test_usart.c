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
	init_rs232();
	enable_rs232_interrupts();
	while(1);
	return 0;
}

void vApplicationTickHook()
{}