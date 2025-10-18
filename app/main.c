#include <stdio.h>
#include "stm32f4xx.h"
#include "usart.h"
#include "delay.h"

extern void board_lowlevel_init(void);

void main(void)
{
	board_lowlevel_init();
	usart_init();
	tim_init();
	
	while(1)
	{
		printf("Hello world!\r\n");
		delay_ms(1000);
	}
}