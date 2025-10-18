#ifndef __USART_H__
#define __USART_H__

#include "stm32f4xx.h"
#include <stdint.h>

typedef void (*usart_receive_callback_t)(uint8_t data);

void usart_init(void);
void usart_send_byte(uint8_t byte);
void usart_send_data(const char *data);
void usart_receive_register(usart_receive_callback_t callback);

#endif