#define USE_STDPERIPH_DRIVER
#include "stm32f10x.h"

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <string.h>

/* Filesystem includes */
#include "filesystem.h"
#include "fio.h"

/* Linenoise and shell includes. */
#include "linenoise.h"

extern const char _sromfs;

static void setup_hardware();

volatile xSemaphoreHandle serial_tx_wait_sem = NULL;
xSemaphoreHandle receive_byte_sem = NULL;

volatile char receive_char = 0;

/* IRQ handler to handle USART2 interruptss (both transmit and receive
 * interrupts). */
void USART2_IRQHandler()
{
	static signed portBASE_TYPE xHigherPriorityTaskWoken;

	/* If this interrupt is for a transmit... */
	if (USART_GetITStatus(USART2, USART_IT_TXE) != RESET) {
		/* "give" the serial_tx_wait_sem semaphore to notfiy processes
		 * that the buffer has a spot free for the next byte.
		 */
		xSemaphoreGiveFromISR(serial_tx_wait_sem, &xHigherPriorityTaskWoken);

		/* Disables the transmit interrupt. */
		USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
		/* If this interrupt is for a receive... */
	}
	/* If this interrupt is for a receive... */
	else if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {

		receive_char = USART_ReceiveData(USART2);
		
		/* Enables the transmit interrupt. */
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
	char buf = 0;

	if(xSemaphoreTake(receive_byte_sem, portMAX_DELAY) == pdTRUE) {
		if(receive_char) {
			buf = receive_char;
			receive_char = 0; //Clean the buffer of the USART
		}
		xSemaphoreGive(receive_byte_sem);
	}
	
	return buf;
}

void read_romfs_task(void *pvParameters)
{
	char buf[128];
	size_t count;
	int fd = fs_open("/romfs/test.txt", 0, O_RDONLY);
	do {
		//Read from /romfs/test.txt to buffer
		count = fio_read(fd, buf, sizeof(buf));
		
		//Write buffer to fd 1 (stdout, through uart)
		fio_write(1, buf, count);
	} while (count);
	
	while (1);
}

void queue_str_task(const char *str)
{
	int msg_len = strlen(str);	

	fio_write(1, str, msg_len);
}

void linenoise_completion(const char *buf, linenoiseCompletions *lc) {
    if (buf[0] == 'h') {
	linenoiseAddCompletion(lc,"hello");
        linenoiseAddCompletion(lc,"hello world!");
    }
}

#define DBG_TEST
void shell_task()
{
	char *shell_str;
	
	linenoiseSetCompletionCallback(linenoise_completion);

	while(1) {
		shell_str = linenoise("linenoise > ");
		#ifdef DBG_TEST
		queue_str_task(shell_str);
		queue_str_task("\n\r");
		#endif
	}
}

int main()
{
	init_rs232();
	enable_rs232_interrupts();
	enable_rs232();
	
	fs_init();
	fio_init();
	
	register_romfs("romfs", &_sromfs);
	
	/* Create the queue used by the serial task.  Messages for write to
	 * the RS232. */
	vSemaphoreCreateBinary(serial_tx_wait_sem);

	/* Create the mutex to protect the receive char */
	receive_byte_sem = xSemaphoreCreateMutex();

	/* Basic shell. */
	xTaskCreate(shell_task,
	            (signed portCHAR *) "Shell",
	            512 /* stack size */, NULL, tskIDLE_PRIORITY + 2, NULL);

	/* Start running the tasks. */
	vTaskStartScheduler();

	return 0;
}

void vApplicationTickHook()
{
}
