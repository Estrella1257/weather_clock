#include <stdio.h>
#include "stm32f4xx.h"
#include "main.h"
#include "pages.h"

void main(void)
{
	board_lowlevel_init();
	board_init();

	welcome_page_display();
	delay_ms(1000);
	wifi_page_display();
	delay_ms(1000);
	main_page_display();
	delay_ms(1000);
	error_page_display("wifi init failed!");

	while(1)
	{
		
	}
}