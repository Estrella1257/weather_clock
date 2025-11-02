#include <stdio.h>
#include "stm32f4xx.h"
#include "main.h"
#include "app.h"
#include "pages.h"

static void main_init(void *param)
{
	board_init();

	welcome_page_display();

	wifi_init();
	wifi_page_display();
	wifi_wait_connected();

	main_page_display();
	main_loop_init();

	vTaskDelete(NULL);
}

int main(void)
{
	board_lowlevel_init();

	xTaskCreate(main_init, "init", 1024, NULL, 9, NULL);

	vTaskStartScheduler();

	while (1)
	{
		;//code should not run here
	}
	
}