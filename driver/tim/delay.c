#include "stm32f4xx.h"
#include "delay.h"

static volatile uint64_t tim_tick_count = 0;       // 全局 tick
static tim_periodic_callback_t periodic_callback = NULL;

void tim2_init(void)
{
    TIM_TimeBaseInitTypeDef TIM_InitStructure;
    TIM_InitStructure.TIM_Period = 999;    // 自动重装载值，1ms
    TIM_InitStructure.TIM_Prescaler = (SystemCoreClock / 1000000) - 1; // 设置预分频器 1MHz 微秒级
    TIM_InitStructure.TIM_ClockDivision = TIM_CKD_DIV1; // 设置时钟分频 TIM_CKD_DIV1：不分频
    TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Up; //向上计数

    TIM_TimeBaseInit(TIM2, &TIM_InitStructure);

    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_Cmd(TIM2, ENABLE);
}

void tim_register_callback(tim_periodic_callback_t cb)
{
    periodic_callback = cb;
}

void delay_us(uint32_t us)
{
    uint64_t target = tim_tick_count * 1000 + us; // tick_count 单位是 ms，乘 1000 转成 us
    while ((tim_tick_count * 1000) < target);
}

void delay_ms(uint32_t ms)
{
    uint64_t target = tim_tick_count + ms;
    while (tim_tick_count < target);
}

void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

        tim_tick_count += 1;                 // 每 1ms 累加 1

        if (periodic_callback)               // 调用周期回调
            periodic_callback();
    }
}