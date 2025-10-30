#include "usart.h"
#include <stdio.h>
#include <string.h>

static usart_receive_callback_t receive_callback = NULL;

static void usart_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_High_Speed;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);
}

static void usart_dma_init(void)
{
    DMA_InitTypeDef DMA_InitStructure;
    DMA_StructInit(&DMA_InitStructure);

    DMA_DeInit(DMA2_Stream7); // 复位DMA2 Stream7到默认状态
    while(DMA_GetCmdStatus(DMA2_Stream7) != DISABLE); // 等待DMA Stream被禁用

    DMA_InitStructure.DMA_Channel = DMA_Channel_4;
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DR; // 设置外设地址为USART1数据寄存器地址
    DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral; // 设置传输方向：内存到外设
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; // 外设地址不递增
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; // 内存地址递增（逐个发送数据缓冲区中的字节）
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal; // DMA模式：正常模式（非循环模式）
    DMA_InitStructure.DMA_Priority = DMA_Priority_Low;  // DMA优先级：低优先级
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable; // 禁用FIFO模式
    DMA_ITConfig(DMA2_Stream7, DMA_IT_TC, ENABLE);
    DMA_Init(DMA2_Stream7, &DMA_InitStructure);

    USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE); // 使能USART1的DMA发送请求
}

static void usart_usart_init(void)
{
    USART_InitTypeDef USART_InitStructure;
    USART_StructInit(&USART_InitStructure);
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART1, &USART_InitStructure);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART1, ENABLE);
}

static void usart_nvic_init(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    memset(&NVIC_InitStructure, 0, sizeof(NVIC_InitTypeDef));
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    NVIC_SetPriority(USART1_IRQn, 5);

    NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream7_IRQn;

    NVIC_Init(&NVIC_InitStructure);
    NVIC_SetPriority(DMA2_Stream7_IRQn, 5);

}

void usart_init(void)
{
    usart_gpio_init();
    usart_dma_init();
    usart_usart_init();
    usart_nvic_init();
}

void usart_send_byte(uint8_t byte)
{
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, byte);
}

void usart_send_data(const char *data)
{
    uint32_t len = strlen(data);
    if (len == 0) return;

    DMA_Cmd(DMA2_Stream7, DISABLE); // 禁用DMA Stream
    while (DMA_GetCmdStatus(DMA2_Stream7) != DISABLE); // 等待DMA Stream被完全禁用

    DMA2_Stream7->M0AR = (uint32_t)data; // 设置内存地址（要发送的数据缓冲区地址）
    DMA2_Stream7->NDTR = len; // 设置要传输的数据数量

    DMA_Cmd(DMA2_Stream7, ENABLE); // 使能DMA Stream开始传输

    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET); 
    USART_ClearFlag(USART1, USART_FLAG_TC);
}

int fputc(int c, FILE *stream)
{
    (void)stream;

    char ch = (char)c;
    usart_send_data(&ch);

    return c;
}

void usart_receive_register(usart_receive_callback_t callback)
{
    receive_callback = callback;
}

void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_FLAG_RXNE) != RESET)
    {
        uint8_t data = (uint8_t)USART_ReceiveData(USART1);
        if(receive_callback)
        {
            receive_callback(data);
        }
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}

void DMA2_Stream7_IRQHandler(void)
{
    if (DMA_GetITStatus(DMA2_Stream7, DMA_IT_TCIF7) != RESET) // 检查DMA2 Stream7的传输完成中断标志是否置位
    {
        DMA_ClearITPendingBit(DMA2_Stream7, DMA_IT_TCIF7); // 清除传输完成中断标志位
    }
}