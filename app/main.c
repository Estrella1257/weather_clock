#include <stdio.h>
#include "stm32f4xx.h"
#include "usart.h"
#include "delay.h"
#include "aht20.h"

extern void board_lowlevel_init(void);

void main(void)
{
	board_lowlevel_init();
	usart_init();
	tim_init();
	aht20_init();
	
	while(1)
	{
		static float last_temperature, last_humidity;
		if(!aht20_start_measurement())
		{
			printf("[AHT20] start measurement failed\n");
			return;
		}

		if(!aht20_wait_for_measurement())
		{
			printf("[AHT20] wait for measurement failed\n");
			return;
		}

		float temperature = 0.0f, humidity = 0.0f;

		if(!aht20_read_measurement(&temperature,&humidity))
		{
			printf("[AHT20] read measurement failed\n");
			return;
		}

		if(last_temperature != temperature && last_humidity == humidity)
		{
			return;
		}

		last_temperature = temperature;
		last_humidity = humidity;

		printf("[AHT20] Temperature: %.1f, Humidity: %.1f\n",temperature,humidity);
	}
}