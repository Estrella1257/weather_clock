#include "esp_at.h"
#include "delay.h"

#define ESP_AT_DEBUG 0        // 用于开启/关闭调试打印（0 关闭，1 开启）
#define USE_DMA_SEND 1        // 是否使用 DMA 发送（1 使用 DMA，0 使用轮询发送）

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))     // 计算数组元素个数的通用宏

/* 接收到 ESP AT 固定响应后，用枚举表示类型，便于后续逻辑判断 */
typedef enum
{
    ESP_AT_ACK_NONE,     // 未知 / 无匹配
    ESP_AT_ACK_OK,       // "OK\r\n" —— 命令成功
    ESP_AT_ACK_ERROR,    // "ERROR\r\n" —— 命令失败
    ESP_AT_ACK_BUSY,     // "busy p...\r\n" —— 模块忙，需要重试
    ESP_AT_ACK_READY,    // "ready\r\n" —— 模块重启完成或就绪
} esp_at_ack_t;

/* ACK 匹配表结构：把字符串与枚举绑定，方便通过 strstr 匹配返回枚举 */
typedef struct 
{
   esp_at_ack_t ack;     // 对应的枚举值
   const char *string;   // 在接收行中匹配的字符串（包含 \r\n）
} esp_at_ack_match_t;

/* 静态的匹配表：按常见的 AT 固定回复填入 */
static const esp_at_ack_match_t ack_matches[] = 
{
    {ESP_AT_ACK_OK, "OK\r\n"},
    {ESP_AT_ACK_ERROR, "ERROR\r\n"},
    {ESP_AT_ACK_BUSY, "busy p...\r\n"},
    {ESP_AT_ACK_READY, "ready\r\n"},
};

static esp_at_receive_callback_t receive_callback = NULL;

static void esp_at_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_StructInit(&GPIO_InitStruct); 
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_Speed = GPIO_High_Speed;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);
}

static void esp_at_usart_init(void)
{
    USART_InitTypeDef USART_InitStruct;
    USART_StructInit(&USART_InitStruct);
    USART_InitStruct.USART_BaudRate = 115200;
    USART_InitStruct.USART_WordLength = USART_WordLength_8b;
    USART_InitStruct.USART_StopBits = USART_StopBits_1;
    USART_InitStruct.USART_Parity = USART_Parity_No;
    USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART2, &USART_InitStruct);
    USART_DMACmd(USART2, USART_DMAReq_Tx, ENABLE);

    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART2, ENABLE);
}

static void esp_at_nvic_init(void)
{
    NVIC_InitTypeDef NVIC_InitStruct;
    memset(&NVIC_InitStruct, 0, sizeof(NVIC_InitStruct));
    NVIC_InitStruct.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 5;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
    NVIC_SetPriority(USART2_IRQn, 5);
}

static void esp_at_dma_init(void)
{
    DMA_InitTypeDef DMA_InitStructure;
    DMA_StructInit(&DMA_InitStructure);

    /* 先复位 DMA 流，确保处于禁能状态后再配置 */
    DMA_DeInit(DMA1_Stream6);
    while (DMA_GetCmdStatus(DMA1_Stream6) != DISABLE);

    DMA_InitStructure.DMA_Channel = DMA_Channel_4;       // 通道号（与 USART2_TX 匹配）
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART2->DR;        // 外设地址：串口数据寄存器
    DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;         // 内存到外设（发送）
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;         // 外设地址不自增
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;          // 内存地址自增
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;        // 1 字节传输
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;           // 正常模式（不是循环）
    DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;         // 优先级
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;          // FIFO 禁用（简化）
    DMA_Init(DMA1_Stream6, &DMA_InitStructure);
}

static void esp_at_lowlevel_init(void)
{
    esp_at_gpio_init();
    esp_at_dma_init();
    esp_at_usart_init();
    esp_at_nvic_init();
}

static bool esp_at_wait_boot(uint32_t timeout)
{
    for(int i = 0; i < timeout / 100; i++)
    {
        if(esp_at_write_command("AT", 100))
            return true;
        delay_ms(100);
    }
    return false;
}

//上层初始化函数：调用底层初始化并执行基础的 AT 检查与复位、存储配置等 
bool esp_at_init(void)
{
    esp_at_lowlevel_init();

    if (!esp_at_wait_boot(3000))        // 等待最初的 AT 响应
        return false;
    if (!esp_at_write_command("AT+RST", 2000))        // 复位 ESP32（需要等待 ready）
        return false;
    if (!esp_at_write_command("AT+SYSSTORE=0", 2000))          // 关闭系统自动保存（示例）
        return false;
    return true;
}

//发送字节（轮询） 用于串口透传
void esp_at_write_byte(uint8_t byte)
{
    while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
    USART_SendData(USART2, byte);
}

//ACK 匹配函数（行匹配）    通过在一行字符串中查找已知关键字来确定 ACK 类型
static esp_at_ack_t match_internal_ack(const char *string)
{
    for (size_t i = 0; i < ARRAY_SIZE(ack_matches); i++)
    {
        //strstr 用于查找模式字符串，若找到则返回对应枚举
        if (ack_matches[i].string && strstr(string, ack_matches[i].string))
        {
            return ack_matches[i].ack;
        }
    }
    return ESP_AT_ACK_NONE;
}

static char rxbuf[1024];        //全局接收缓冲区，保存自上次命令起收到的所有字节（用于上层解析/返回）
static char rxlinebuf[256];     //用于按行解析（检测换行 '\n' 时判断一行结束）
static size_t rxlen = 0;        //当前长度索引，rxline_idx: rxlinebuf 当前索引
static size_t rxline_idx = 0; 
static volatile bool at_ack_flag = false;        //“中断侧通知主循环/写命令等待”的标志位 
static esp_at_ack_t rxack = ESP_AT_ACK_NONE;         //最近匹配到的 ACK 类型（OK/ERROR/BUSY/READY）

//接收解析函数（每收到一个字节调用）
//该函数在 USART RX 中断中被调用（或轮询读取到字节时也可调用）
static void esp_at_receive_parser(uint8_t data)
{
    //把字节追加到行缓冲，保证以 '\0' 结尾 
    if (rxline_idx < sizeof(rxlinebuf) - 1)
    {
        rxlinebuf[rxline_idx++] = (char)data;
        rxlinebuf[rxline_idx] = '\0';
    }

    //同时追加到全局 rxbuf，以便上层获取整段响应文本 
    if (rxlen < sizeof(rxbuf) - 1)
    {
        rxbuf[rxlen++] = (char)data;
        rxbuf[rxlen] = '\0';
    }

    //若用户注册了字节级回调，则在此调用，允许外部处理每个字节 
    if (receive_callback)
    {
        receive_callback(data);
    }

    //当遇到换行符时，尝试匹配行内的 ACK（OK/ERROR 等），匹配到则置标志并保存类型
    if (data == '\n')
    {
        esp_at_ack_t ack = match_internal_ack(rxlinebuf);
        if (ack != ESP_AT_ACK_NONE)
        {
            rxack = ack;
            at_ack_flag = true;
        }
        
        //清空行缓冲，准备下一行
        rxline_idx = 0;
        rxlinebuf[0] = '\0';
    }
}

//将一个字符串通过 DMA 或轮询发送到 USART（字符串不包含结尾 \r\n 时上层通常会加）
static void esp_at_usart_write(const char *data)
{
    #if USE_DMA_SEND
        uint32_t len = strlen(data);
        if (len == 0) return;
        //直接写 DMA 的内存地址与传输长度，启动 DMA 传输 
        DMA1_Stream6->M0AR = (uint32_t)data;        // 数据源地址（仅在内存连续且不可修改时安全）
        DMA1_Stream6->NDTR = len;                   // 传输字节数
        DMA_ClearFlag(DMA1_Stream6, DMA_FLAG_TCIF6);
        DMA_Cmd(DMA1_Stream6, ENABLE);
    #else
        //轮询逐字节发送
        while (*data)
        {
            while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET) { }
            USART_SendData(USART2, (uint8_t)(*data++));
        }
    #endif
}

//将传入的字符串（不带 CRLF）拼接上 "\r\n" 后发送（用于发送 AT 命令）
static void esp_at_write_bytes(const char *data)
{
    char tmp[256];
    size_t l = snprintf(tmp, sizeof(tmp), "%s\r\n", data);
    if (l >= sizeof(tmp)) l = sizeof(tmp) - 1;      // 防止越界
    esp_at_usart_write(tmp);
}

//发字符串并等待响应
/*
  逻辑：
    - 每次发送命令前清空 rxbuf、rxack、标志位
    - 发送命令（通过 DMA 或轮询）
    - 轮询检查 at_ack_flag（由中断在接收到包含关键字的一行时置位）
    - 根据 rxack 类型返回 true/false，或在 BUSY 时重试，超时则失败
  timeout_ms: 总超时时间（毫秒）
*/
static bool esp_at_write_command(const char *command, uint32_t timeout_ms)
{
    rxbuf[0] = '\0';
    rxlen = 0;
    rxack = ESP_AT_ACK_NONE;

#if USE_DMA_SEND
    //若使用 DMA，构建一个静态缓冲（注意：如果命令长度动态变化且高频调用，需注意数据覆盖）
    static char cmd_buf[512];
    int l = snprintf(cmd_buf, sizeof(cmd_buf), "%s\r\n", command);
    if (l <= 0) return false;
    esp_at_usart_write(cmd_buf);
#else
    esp_at_write_bytes(command);
#endif

    uint32_t elapsed = 0;
    while (elapsed < timeout_ms)
    {
        if (at_ack_flag)
        {
            //收到标志：根据 rxack 判断结果
            if (rxack == ESP_AT_ACK_OK || rxack == ESP_AT_ACK_READY)
                return true;
            else if (rxack == ESP_AT_ACK_ERROR)
                return false;
            else if (rxack == ESP_AT_ACK_BUSY)
            {
                //BUSY：清标志、短延时后重试发送
                at_ack_flag = false;
                delay_ms(50);
                esp_at_usart_write(cmd_buf);
            }
        }
        delay_ms(5);
        elapsed += 5;

#if ESP_AT_DEBUG
    printf("AT cmd timeout: %s\n", command);
#endif
    return false;
    }
}

//返回指向全局 rxbuf 的只读字符串（注意：缓冲会被后续命令覆盖）
const char *esp_at_get_response(void)
{
    return rxbuf;
}

//WiFi初始化,设置为 station 模式（1）
bool esp_at_wifi_init(void)
{
    return esp_at_write_command("AT+CWMODE=1", 2000);
}

//连接WiFi
bool esp_at_connect_wifi(const char *ssid, const char *pwd,const char *mac)
{
    if(ssid == NULL || pwd == NULL) 
       return false;

    char command[128];
    //构造 AT+CWJAP="ssid","pwd" 或带 MAC 的扩展命令
    int len = snprintf(command, sizeof(command), "AT+CWJAP=\"%s\",\"%s\"", ssid, pwd);
    if(mac)
        snprintf(command + len, sizeof(command) - len, ",\"%s\"", mac);
    
    return esp_at_write_command(command, 10000);
}

//解析 +CWSTATE 响应（判断是否连接）
static bool parse_cwstate_response(const char *response,esp_at_wifi_info_t *info)
{
//     AT+CWSTATE?
// +CWSTATE:2,"Redmi K60 Pro"

//OK
    response = strstr(response, "+CWSTATE:");
    if(response == NULL)
        return false;

    int wifi_state;
    //sscanf 解析：第一个整数为 state，第二个字符串为 ssid（最多 63 字节）
    if(sscanf(response,"+CWSTATE:%d,\"%63[^\"]",&wifi_state,info->ssid) != 2)
        return false;

    info->connected = (wifi_state == 2);   // state==2 常表示已连接（取决于固件）
    return true;
}

//解析 +CWJAP 响应（获得详细 AP 信息）
static bool parse_cwjap_response(const char *response,esp_at_wifi_info_t *info)
{
// AT+CWJAP?
// +CWJAP:"Redmi K60 Pro","ee:2a:fb:2b:e9:9a",11,-33,0,1,3,0,1

//OK
    response =strstr(response, "+CWJAP:");
    if(response == NULL)
        return false;
    
    //解析 ssid, bssid, channel, rssi（格式需和固件输出一致）
    if(sscanf(response,"+CWJAP:\"%63[^\"]\",\"%17[^\"]\",%d,%d",info->ssid,info->bssid,&info->channel,&info->rssi) != 4)
        return false;

    return true;
}

//获取 WiFi 信息（组合调用上面两个解析函数）
bool esp_at_get_wifi_info(esp_at_wifi_info_t *info)
{
    if(!esp_at_write_command("AT+CWSTATE?", 2000))
        return false;

    if(!parse_cwstate_response(esp_at_get_response(),info))
        return false;

    if (info->connected)
    {
        if (!esp_at_write_command("AT+CWJAP?", 2000))
            return false;
        if (!parse_cwjap_response(esp_at_get_response(), info))
            return false;
    }

    return true;
}

//wifi_is_connected 辅助函数
bool wifi_is_connected(void)
{
    esp_at_wifi_info_t info;
    if (esp_at_get_wifi_info(&info))
        return info.connected;

    return false;
}

//SNTP 初始化（设置时区、启用 SNTP）
bool esp_at_sntp_init(void)
{
    //这里使用 AT+CIPSNTPCFG=1,8 —— 1 表示启用，8 表示时区（+8），根据模块固件不同参数或有差异 
    return esp_at_write_command("AT+CIPSNTPCFG=1,8", 2000);
}

//字符串月/星期转换
//把英文缩写月/周字符串转为数字，方便转换成日期结构;month_str: "Jan"..."Dec" 返回 1..12，失败返回 0
static uint8_t month_str_to_num(const char *month_str)
{
    const char *months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    for (uint8_t i = 0; i < ARRAY_SIZE(months); i++)
    {
        if (strcmp(month_str, months[i]) == 0)
            return i + 1;
    }
    return 0;
}

//weekday_str: "Mon".."Sun" 返回 1..7（以 Mon 为 1），失败返回 0
static uint8_t weekday_str_to_num(const char *weekday_str)
{
    const char *weekdays[] = {
        "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"
    };
    for (uint8_t i = 0; i < ARRAY_SIZE(weekdays); i++)
    {
        if (strcmp(weekday_str, weekdays[i]) == 0)
            return i + 1;
    }
    return 0;
}

//解析 SNTP 返回时间（AT+CIPSNTPTIME?）
static bool parse_cipsntptime_response(const char *response, esp_at_datetime_t *date)
{
//	AT+CIPSNTPTIME?
//	+CIPSNTPTIME:Sun Jul 27 14:07:19 2025
//	OK
    char weekday_str[8];
    char month_str[4];
    response = strstr(response, "+CIPSNTPTIME:");
    if (response == NULL) 
    {
        return false;
    }
    //sscanf 提取 weekday（3 字母）、month（3 字母）、day（%hhu）、hour:min:sec、year（%hu）
    if(sscanf(response, "+CIPSNTPTIME:%3s %3s %hhu %hhu:%hhu:%hhu %hu",
               weekday_str, month_str, &date->day, &date->hour, &date->minute, &date->second, &date->year) != 7)
    {
        return false;
    }

    //转换英文缩写到数字
    date->month = month_str_to_num(month_str);
    date->weekday = weekday_str_to_num(weekday_str);
    return true;
}

//获取 SNTP 时间（发送查询命令并解析）
bool esp_at_sntp_get_time(esp_at_datetime_t *date)
{
    if(!esp_at_write_command("AT+CIPSNTPTIME?", 2000))
        return false;

    if(!parse_cipsntptime_response(esp_at_get_response(), date))
        return false;

    return true;
}

//使用 AT+HTTPCLIENT=2,1,"<url>",,,2 这种固件自带的 HTTP 客户端（具体参数依固件而定）
//函数返回指向 rxbuf 的指针（注意返回值在随后的命令调用时会被覆盖）
const char *esp_at_http_get(const char *url)
{
//     AT+HTTPCLIENT=2,1,"https://api.seniverse.com/v3/weather/now.json?key=SU8PEBzZMEJ22jpWz&location=hefei&language=en&unit=c",,,2
// +HTTPCLIENT:261,{"results":[{"location":{"id":"WTEMH46Z5N09","name":"Hefei","country":"CN","path":"Hefei,Hefei,Anhui,China","timezone":"Asia/Shanghai","timezone_offset":"+08:00"},"now":{"text":"Cloudy","code":"4","temperature":"29"},"last_update":"2025-08-31T13:34:06+08:00"}]}

//OK
    char txbuf[256];
    snprintf(txbuf, sizeof(txbuf), "AT+HTTPCLIENT=2,1,\"%s\",,,2", url);
    bool ret = esp_at_write_command(txbuf, 7000);
    if (ret) {
        delay_ms(100);      // 额外短延时，确保模块把 HTTP 数据发送完毕并进入 rxbuf
        return esp_at_get_response();       // 返回 rxbuf（包含 +HTTPCLIENT:... 的 JSON 数据）
    }
    return NULL;
}

void esp_at_receive_register(esp_at_receive_callback_t callback)
{
    receive_callback = callback;
}

void USART2_IRQHandler(void)
{
    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        uint8_t data = (uint8_t)USART_ReceiveData(USART2);
        esp_at_receive_parser(data);        // 解析处理该字节（非阻塞）
        USART_ClearITPendingBit(USART2, USART_IT_RXNE);
    }
}