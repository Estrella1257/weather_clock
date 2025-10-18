#include <stdio.h>
#include "stm32f4xx.h"
#include "usart.h"

extern void board_lowlevel_init(void);

void main(void)
{
	board_lowlevel_init();
	usart_init();
	printf("Hello world!\r\n");
	
	while(1)
	{
		
	}
}