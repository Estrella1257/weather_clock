#ifndef __DELAY_H__
#define _DELAY_H__

#include <stdint.h>

void tim_init(void);
void delay_us(uint32_t us);
void delay_ms(uint32_t ms);

#endif