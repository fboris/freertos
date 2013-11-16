#define USE_STDPERIPH_DRIVER
#include "stm32f4xx_conf.h"
#include "stm32f4_discovery.h"
/*Serial IO*/
#include "serial_io.h"
/*FreeRTOS relative */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
/* Filesystem relative */
#include "filesystem.h"
#include "fio.h"
#include "romfs.h"
/*shell enviroment*/
#include "shell.h"

//extern const char _sromfs;

volatile xSemaphoreHandle serial_tx_wait_sem = NULL;
volatile xQueueHandle serial_rx_queue = NULL;
int main()
{
	init_rs232();
	enable_rs232_interrupts();

    fs_init();
    fio_init();
    //FIXME: romfs complilation error    
    //register_romfs("romfs", &_sromfs);
        
    /* Create the queue used by the serial task. Messages for write to
     * the RS232. */
    vSemaphoreCreateBinary(serial_tx_wait_sem);

    serial_rx_queue = xQueueCreate(1, sizeof(serial_msg));
    /* Create a shell task */
    xTaskCreate(shell_task,
    (signed portCHAR *) "Shell ENV",
    512 /* stack size */, NULL, tskIDLE_PRIORITY + 5, NULL);

    /* Start running the tasks. */
    vTaskStartScheduler();

	return 0;
}

void vApplicationTickHook()
{}