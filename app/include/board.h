#ifndef __BOARD_H
#define __BOARD_H

#include <stdio.h>
#include "stm32f4xx.h"
#include "usart.h"
#include "delay.h"
#include "aht20.h"
#include "lcd.h"
#include "esp_at.h"

void board_lowlevel_init(void);
void board_init(void);

#endif