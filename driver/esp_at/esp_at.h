#ifndef __ESP_AT_H__
#define __ESP_AT_H__

#include "stm32f4xx.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>

typedef void (*esp_at_receive_callback_t)(uint8_t data);

/* WiFi 信息结构体：
   - ssid: AP 名称（64 字节）
   - bssid: MAC（18 字符串，格式 "xx:xx:..."）
   - channel, rssi: 通道与信号强度（注意这里用 char 存储，若超出范围需改为 int）
   - connected: 是否已连接标志
   注意：此处字段使用 const char / const char 的写法在实际填充时不合适（const 表示只读）；
         你源码中使用 sscanf 填充这些字段，建议把类型改为非 const，例如 char ssid[64];
*/
typedef struct
{
    const char ssid[64];
    const char bssid[18];
    const char channel;
    const char rssi;
    bool connected;
} esp_at_wifi_info_t;

//日期时间结构体：用于 SNTP 时间解析与返回
typedef struct
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t weekday;
} esp_at_datetime_t;

bool esp_at_init(void);
void esp_at_write_byte(uint8_t byte);
bool esp_at_write_command(const char *command,uint32_t timeout);
const char *esp_at_get_response(void);
bool esp_at_wifi_init(void);
bool esp_at_connect_wifi(const char *ssid, const char *pwd,const char *mac);
bool esp_at_get_wifi_info(esp_at_wifi_info_t *info);
bool wifi_is_connected(void);
bool esp_at_sntp_init(void);
bool esp_at_sntp_get_time(esp_at_datetime_t *date);
void esp_at_receive_register(esp_at_receive_callback_t callback);
const char *esp_at_http_get(const char *url);


#endif