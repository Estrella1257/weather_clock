#include "aht20.h"
#include "stm32f4xx.h"
#include "delay.h"

static bool aht20_write(uint8_t data[], uint32_t length);
static bool aht20_read(uint8_t data[], uint32_t length);
static bool aht20_is_ready(void);

bool aht20_init(void)
{
    I2C_DeInit(I2C2);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_StructInit(&GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD; // 开漏输出（I2C要求）
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL; // 不上拉（I2C线外部有上拉）
    GPIO_InitStructure.GPIO_Speed = GPIO_High_Speed;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_I2C2);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_I2C2);

    I2C_InitTypeDef I2C_InitStructure;
    I2C_StructInit(&I2C_InitStructure);
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable; // 打开应答
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit; // 7位地址模式
    I2C_InitStructure.I2C_ClockSpeed = 100ul * 1000ul; //100kHz
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2; // 标准模式
    I2C_InitStructure.I2C_OwnAddress1 = 0x00; //主机地址
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_Init(I2C2,&I2C_InitStructure);
    I2C_Cmd(I2C2, ENABLE);

    delay_ms(40);

    if (aht20_is_ready()) return true; //检查传感器是否已准备好（是否已初始化）
    if (!aht20_write((uint8_t[]){0xBE, 0x08, 0x00},3)) return true; //若未准备，则发送初始化命令（0xBE, 0x08, 0x00）

    for (uint32_t t = 0; t < 100; t++) //等待传感器初始化完成
    {
        delay_ms(1);
        if (aht20_is_ready()) return true;    
    }
    return false;
}

// 宏定义：检查 I2C 事件是否发生（带超时机制）
#define I2C_CHECK_EVENT(EVENT, TIMEOUT) \
    do { \
        uint32_t timeout = TIMEOUT; \
        while (!I2C_CheckEvent(I2C2, EVENT) && timeout > 0) { \
            delay_ms(10); \
            timeout -= 10; \
        } \
        if (timeout <= 0) \
            return false; \
    } while (0)


static bool aht20_write(uint8_t data[], uint32_t length)
{
    I2C_AcknowledgeConfig(I2C2, ENABLE);  //使能 I2C 应答（主机接收数据时自动应答）
    I2C_GenerateSTART(I2C2, ENABLE); 
    I2C_CHECK_EVENT(I2C_EVENT_MASTER_MODE_SELECT, 1000); //等待 I2C 进入 “主机模式选择” 状态,若超时则会失败
    I2C_Send7bitAddress(I2C2, 0x70, I2C_Direction_Transmitter); // AHT20 地址为 0x38 << 1 = 0x70,第二个参数表示方向：Transmitter（写）
    I2C_CHECK_EVENT(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED, 1000); //等待从机 ACK 地址并进入发送器模式,若超时或从设备未 ACK（NACK）则会失败
    for (uint32_t i = 0; i < length; i++)
    {
        I2C_SendData(I2C2, data[i]);
        I2C_CHECK_EVENT(I2C_EVENT_MASTER_BYTE_TRANSMITTING, 1000); //等待“主机正在发送字节”事件,传输已开始,I2C_EVENT_MASTER_BYTE_TRANSMITTED（表示字节已完全发送并被移出移位寄存器）
    }
    I2C_GenerateSTOP(I2C2, ENABLE); 

    return true;
}

static bool aht20_read(uint8_t data[], uint32_t length)
{
    I2C_AcknowledgeConfig(I2C2, ENABLE);
    I2C_GenerateSTART(I2C2, ENABLE);
    I2C_CHECK_EVENT(I2C_EVENT_MASTER_MODE_SELECT, 1000);
    I2C_Send7bitAddress(I2C2, 0x70, I2C_Direction_Receiver); 
    I2C_CHECK_EVENT(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED, 1000);
    for (uint32_t i = 0; i < length; i++)
    {
        if (i == length - 1) I2C_AcknowledgeConfig(I2C2, DISABLE); // 最后一个字节不应答
        I2C_CHECK_EVENT(I2C_EVENT_MASTER_BYTE_RECEIVED, 1000); //主机在接收模式下已经成功接收到一个字节的数据，且该字节已保存在 DR（数据寄存器）中，等待 CPU 读取
        data[i] = I2C_ReceiveData(I2C2);
    }
    I2C_GenerateSTOP(I2C2, ENABLE);

    return true;
}

static bool aht20_read_status(uint8_t *status)
{
    uint8_t cmd = 0x71; // 读取状态命令
    if (!aht20_write(&cmd, 1)) return false;
    if (!aht20_read(status, 1)) return false;
    return true;
}

static bool aht20_is_busy(void)
{
    uint8_t status;
    if (!aht20_read_status(&status)) return false;
    return (status & 0x80) != 0; // bit7=1 表示忙
}

static bool aht20_is_ready(void)
{
    uint8_t status;
    if (!aht20_read_status(&status)) return false;
    return (status & 0x08) != 0; // bit3=1 表示已初始化
}

bool aht20_start_measurement(void)
{
    return aht20_write((uint8_t[]){0xAC, 0x33, 0x00},3); //发送测量命令（0xAC, 0x33, 0x00）
}

bool aht20_wait_for_measurement(void)
{
    for (uint32_t t = 0; t < 100; t++)
    {
        delay_ms(1);
        if (!aht20_is_busy()) return true;
    }
    return false;
}

bool aht20_read_measurement(float *tempreature, float *humidity)
{
    uint8_t data[6];
    if (!aht20_read(data, 6)) return false;
    // 湿度数据20位
    uint32_t raw_humidity = ((uint32_t)data[1] << 12) |
                                ((uint32_t)data[2] << 4) |
                                ((uint32_t)(data[3] & 0xF0) >> 4);
    // 温度数据20位
    uint32_t raw_tempreature = ((uint32_t)(data[3] & 0x0F) << 16) |
                                    ((uint32_t)data[4] << 8) |
                                    ((uint32_t)data[5]);
     // 转换为实际物理量
    *humidity = (float)raw_humidity * 100.0f / (float)0x100000;
    *tempreature = (float)raw_tempreature * 200.0f / (float)0x100000 - 50.0f;

    return true;
}