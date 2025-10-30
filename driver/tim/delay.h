#ifndef __DELAY_H__
#define __DELAY_H__

typedef void (*tim_periodic_callback_t)(void);

#include <stdint.h>
#include <stddef.h>

void tim2_init(void);
void delay_us(uint32_t us);
void delay_ms(uint32_t ms);
void tim_register_callback(tim_periodic_callback_t cb);

#endif