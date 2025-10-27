#include "board.h"

void board_lowlevel_init(void)
{
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
    RCC_AHB3PeriphClockCmd(RCC_AHB3Periph_FSMC,ENABLE); //使能FSMC时钟  


    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
}

static void usart_received(uint8_t data)
{
    esp_at_write_byte(data);
}

static void esp_at_received(uint8_t data)
{
    usart_send_byte(data);
}

void board_init(void)
{
    tim_init();
    usart_init();
    lcd_init();
    aht20_init();
    usart_receive_register(usart_received);
	esp_at_receive_register(esp_at_received);
}
