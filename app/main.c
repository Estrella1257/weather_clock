#include <stdio.h>
#include "stm32f4xx.h"
#include "main.h"

void main(void)
{
	board_lowlevel_init();
	board_init();

	lcd_clear(RED);
	delay_ms(2000);
	lcd_show_char(0, 0, 'a', &font16_maple, 1, YELLOW, BLACK);
	lcd_show_string(0, 16, "123abc*&/?", &font16_maple, 1, YELLOW, BLACK);
	lcd_show_string(0, 24, "23 -", &font62_maple, 1, BLACK, BLACK);
	lcd_show_string(0, 57, "123", &font76_maple, 1, BLACK, BLACK);
	lcd_show_string(0, 110, "12:", &font54_maple, 1, BLACK, BLACK);
	lcd_show_string(0, 150, "天气时钟连接中℃", &font32_maple, 1, BLACK, BLACK);
	lcd_show_string(0, 205, "一二三四五六日星期", &font20_maple, 1, BLACK, BLACK);
	lcd_show_chinese_char(72, 0, "梅", &font32_maple, 1, BLUE, BLACK);

	printf("[SYS] Build Date: %s %s\n", __DATE__, __TIME__);
	
    if(!esp_at_init())
	{
		printf("[AT] init failed!\n");
		
	}
	printf("[AT] inited!\n");
	
	if (!esp_at_wifi_init())
	{
		printf("[WIFI] init failed!\n");
		
	}
	printf("[WIFI] inited!\n");	

	if (!esp_at_connect_wifi("Redmi K60 Pro","12345678", NULL))
	{
		printf("[WIFI] connect failed!\n");
		
	}
	printf("[WIFI] connecting...\n");

	if (!esp_at_sntp_init())
	{
		printf("[SNTP] init failed!\n");
		
	}
	printf("[SNTP] inited!\n");

	return;

	
	while(1)
	{
		
	}
}