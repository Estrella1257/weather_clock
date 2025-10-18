#include "stm32f4xx.h"
#include "delay.h"

void tim_init(void)
{
    TIM_TimeBaseInitTypeDef TIM_InitStructure;
    TIM_InitStructure.TIM_Period = 0xFFFFFFFF; // 自动重装载寄存器，最大
    TIM_InitStructure.TIM_Prescaler = (SystemCoreClock / 1000000) - 1; // 设置预分频器 1MHz
    TIM_InitStructure.TIM_ClockDivision = TIM_CKD_DIV1; // 设置时钟分频 TIM_CKD_DIV1：不分频
    TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Up; //向上计数

    TIM_TimeBaseInit(TIM2, &TIM_InitStructure);
    TIM_Cmd(TIM2, ENABLE);
}

void delay_us(uint32_t us)
{
    uint32_t start = TIM_GetCounter(TIM2);
    while ((TIM_GetCounter(TIM2) - start) < us);
}

void delay_ms(uint32_t ms)
{
    while (ms--)
    {
        delay_us(1000);
    }
}