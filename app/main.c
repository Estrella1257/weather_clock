#include <stdio.h>
#include "stm32f4xx.h"
#include "main.h"
#include "app.h"
#include "pages.h"

int main(void)
{
	board_lowlevel_init();
	board_init();

	welcome_page_display();

	wifi_init();
	wifi_page_display();
	wifi_wait_connected();

	main_loop_init();
	main_page_display();

	while (1)
	{
		main_loop();
	}
	
}