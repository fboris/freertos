/*STM Firmware*/
#include "stm32f4xx_conf.h"
#include "stm32f4_discovery.h"
/*FreeRTOS relative */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <string.h>
/* Filesystem relative */
#include "filesystem.h"
#include "fio.h"
/*Serial IO*/
#include "serial_io.h"

extern volatile xSemaphoreHandle serial_tx_wait_sem;
extern volatile xQueueHandle serial_rx_queue;
char getch_base()
{
    char ch;
    fio_read(0, &ch, 1);
    return ch;
}

void putch_base(char ch)
{
    fio_write(1, &ch, 1);
}

/* Serial read/write callback functions */
serial_ops serial = {
    .getch = getch_base,
    .putch = putch_base,
};




/* IRQ handler to handle USART2 interruptss (both transmit and receive
 * interrupts). */
void USART2_IRQHandler()
{
	static signed portBASE_TYPE xHigherPriorityTaskWoken;
	serial_msg rx_msg;
	/* If this interrupt is for a transmit... */
	if (USART_GetITStatus(USART2, USART_IT_TXE) != RESET) {
		/* "give" the serial_tx_wait_sem semaphore to notfiy processes
		 * that the buffer has a spot free for the next byte.
		 */
		xSemaphoreGiveFromISR(serial_tx_wait_sem, &xHigherPriorityTaskWoken);

		/* Diables the transmit interrupt. */
		USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
		/* If this interrupt is for a receive... */
	}
	else if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET){
	       /* Receive the byte from the buffer. */
		rx_msg.ch = USART_ReceiveData(USART2);
	     
	        /* Queue the received byte. */
		if(!xQueueSendToBackFromISR(serial_rx_queue, &rx_msg, &xHigherPriorityTaskWoken)) {
	         /* If there was an error queueing the received byte,
		     * freeze. */
		    taskYIELD();
		}
	}
	else {
		/* Only transmit and receive interrupts should be enabled.
		 * If this is another type of interrupt, freeze.
		 */
		while(1);
	}

	if (xHigherPriorityTaskWoken) {
		taskYIELD();
	}
}

void send_byte(char ch)
{
	/* Wait until the RS232 port can receive another byte (this semaphore
	 * is "given" by the RS232 port interrupt when the buffer has room for
	 * another byte.
	 */
	while (!xSemaphoreTake(serial_tx_wait_sem, portMAX_DELAY));

	/* Send the byte and enable the transmit interrupt (it is disabled by
	 * the interrupt).
	 */
	USART_SendData(USART2, ch);
	USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
}

char receive_byte()
{
	serial_msg msg;
    
	 /* Wait for a byte to be queued by the receive interrupts handler. */
	while (!xQueueReceive(serial_rx_queue, &msg, portMAX_DELAY));
	       
	return msg.ch;
}

