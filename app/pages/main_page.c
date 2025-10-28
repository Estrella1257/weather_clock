#include "lcd.h"
#include "rtc.h"
#include "app.h"
#include <string.h>
#include <stdio.h>
#include <pages.h>

// rtc_datetime_t 结构体用于存放 RTC 获得的时间日期信息。
// 在文件作用域定义一个全局变量 now，便于 main_page_display 和其他子函数读取当前时间。
// 注意：如果系统是多线程/中断访问 RTC，这个全局变量的访问可能需要同步保护（互斥/禁中断等）。
rtc_datetime_t now;  // 当前 RTC 时间（全局变量）

/**
 * 主页界面绘制（一次性初始化/重绘）
 * 负责清屏、绘制各个模块（WiFi、时间/日期、室内信息、室外信息）
 * 说明：do { ... } while (0); 在这里被用于把一组绘制操作包成一个代码块（方便折叠/局部变量/视觉分组）。
 */
void main_page_display(void)
{
    lcd_fill(0, 0, 240, 320, BLACK);

    // 顶部信息区域（白底）
    do
    {
        lcd_fill(15,15,224,154,WHITE);

        lcd_show_image(23, 20, &img_icon_wifi);

        // 在标题区显示 WiFi SSID（使用全局宏/常量 WIFI_SSID）
        main_page_redraw_wifi_ssid(WIFI_SSID);

        // 从 RTC 获取当前时间并保存在全局 now 中
        // rtc_get_time 函数应填充 rtc_datetime_t（请确保 rtc_get_time 返回前 RTC 数据是有效的）
        rtc_get_time(&now);

        // 在屏幕上重绘时间（小时:分钟）
        main_page_redraw_time(&now);

        // 在屏幕上重绘日期（YYYY/MM/DD 星期X）
        main_page_redraw_date(&now);
    }while (0);
    
    // 左下室内环境信息区域（青色背景）
    do 
    {
        lcd_fill(15,165,114,304,TEAL);

        lcd_show_string(25, 170, "室内环境", &font20_maple, 1, BLACK, WHITE);

        // 初始值为 "--"（表示未知/未初始化），这里用两个字符 '-' 填充 char 数组初始化
        main_page_redraw_inner_tempreature('--');
        main_page_redraw_inner_humidity('--');
    }while(0);
    
    // 右下室外信息区域（橙色背景）
    do
    {
        lcd_fill(125,165,224,304,RGB565(253, 135, 75));

        // 绘制城市名
        main_page_redraw_outdoor_city("湛江");

        // 室外温度默认显示 "--"
        main_page_redraw_outdoor_tempreature('--');

        // 初始天气图标为 -1（表示未知），会显示默认 NA 图标
        main_page_redraw_outdoor_weather_icon(-1);
    } while (0);
    
}

/**
 * 在 WiFi 区域更新 SSID 显示
 * 参数：const char *ssid - 要显示的 SSID 字符串
 */
void main_page_redraw_wifi_ssid(const char *ssid)
{
    lcd_fill(50, 23, 200, 43, WHITE);

    // 缓冲区大小 15 字节（包括结尾 '\0'）
    // 我们用 snprintf(str,15,"%14s", ssid) 将 SSID 限制到最多 14 个字符，保证不溢出
    // 注：如果 SSID 比 14 字更长，会被截断；你可根据屏幕宽度调整该值
    char str[15];
    snprintf(str, 15, "%14s", ssid);

    lcd_show_string(50, 23, str, &font20_maple, 1, GREEN, WHITE);
}

/**
 * 在标题区重绘时间（格式 HH:MM）
 * 参数：rtc_datetime_t *time - 指向含有 hour、minute 的时间结构体
 */
void main_page_redraw_time(rtc_datetime_t *time)
{
    lcd_fill(25, 42, 215, 118, WHITE);

    // 缓冲区 6 字节：HH:MM + '\0' 需要 6 字节（例如 "08:05\0"）
    char str[6];
    // 使用 %02u 保证小时和分钟总是两位（0 填充），例如 "09:07"
    // 注意：time->hour/time->minute 的类型应与 %02u 匹配（通常是 unsigned int/uint8_t）
    snprintf(str,sizeof(str),"%02u:%02u",time->hour ,time->minute);

    lcd_show_string(25, 42, str, &font76_maple, 1, BLACK, WHITE);
}

/**
 * 在标题区重绘日期（格式 YYYY/MM/DD 星期X）
 * 参数：rtc_datetime_t *date - 包含 year,month,day,weekday
 */
void main_page_redraw_date(rtc_datetime_t *date)
{
    lcd_fill(35, 121, 55, 141, WHITE);

    // 缓冲区 18 字节（包括 '\0'）：
    // 例如 "2025/10/27 星期一" 长度为 4+1+2+1+2+1+3 = 14（不同语言环境可能略有差别），18 是安全的
    char str[18];

    // weekday 对应中文 "一" 到 "日"。当 weekday 不在 1-7 内时，显示 "X" 作为占位
    // 这里用条件运算符构造星期字符串部分
    snprintf(str,sizeof(str),"%04u/%02u/%02u 星期%s",date->year,date->month,date->day,
                date-> weekday == 1 ? "一" :
                date-> weekday == 2 ? "二" :
                date-> weekday == 3 ? "三" :
                date-> weekday == 4 ? "四" :
                date-> weekday == 5 ? "五" :
                date-> weekday == 6 ? "六" :
                date-> weekday == 7 ? "日" : "X" 
            );

    lcd_show_string(35, 121, str, &font20_maple, 1, BLACK, WHITE);
}

/**
 * 绘制或更新室内温度显示
 * 参数：float tempreature - 温度（摄氏度）；如果传入非有效值（如 '--' 的编码），则显示 "--"
 * 说明：函数内部以 float 为输入，但你实际调用时传入的是 '--'（字符常量），在 C 里 '--' 会被解释为整数常量 —— 这在你的原代码里是一个潜在不清晰点。
 * 建议：最好使用特殊值（如 NAN）或负数区间外的值来表示未知，而不是字符常量。
 */
void main_page_redraw_inner_tempreature(float tempreature)
{
    lcd_fill(30, 195, 100, 249, TEAL);

    // 字符串缓冲区 3 字节，用于存放两位数 + '\0'，例如 "23\0"
    // 初始化为 {'-','-'}，字符串末尾自动被下一行 snprintf 写入 '\0'（但这里手动初始化未包含 '\0' —— 实际 C 规定静态初始化会在尾部补0）
    char str[3] = {'-','-'};

    // 有效温度判断：在 -10.0 到 100.0 摄氏度之间才认为合法并格式化为整数显示
    // 你选择用 %2.0f（四舍五入到整数）并占 2 个字符宽度
    if(tempreature > -10.0f && tempreature <= 100.0f)
        snprintf(str, sizeof(str), "%2.0f", tempreature);

    lcd_show_string(30, 195, str, &font54_maple, 1, BLACK, TEAL);

    lcd_show_string(86, 213, "℃", &font32_maple,1, BLACK, WHITE);    
}

/**
 * 绘制或更新室内湿度显示
 * 参数：float humiduty - 湿度值（%）
 */
void main_page_redraw_inner_humidity(float humiduty)
{
    lcd_fill(28, 239, 110, 301, TEAL);

    // 缓冲区 3 字节足够显示两位整数和终止符
    char str[3];

    // 仅当湿度在 (0, 99.99] 范围内才显示（0 被视为无效/未检测？）
    // 注意：如果真实湿度为 0%，这个判断会把它当作无效。若要允许 0%，应改为 >= 0.0f
    if(humiduty > 0.0f && humiduty <= 99.99f)
        snprintf(str, sizeof(str), "%2.0f", humiduty);

    lcd_show_string(28, 239, str, &font62_maple,1, BLACK, TEAL);

    lcd_show_string(91, 262, "%", &font32_maple,1, BLACK, WHITE);    
}

/**
 * 在室外信息区显示城市名称
 * 参数：const char *city - 城市字符串
 * 说明/问题点：这里函数内有个局部 str[9] 缓冲区但最终并未使用它（直接把 city 传给 lcd_show_string）。
 * 这会造成多余的栈分配；如果想要限制显示长度，应使用 snprintf 填充 str 并传入 str 给 lcd_show_string。
 */
void main_page_redraw_outdoor_city(const char *city)
{
    // 局部缓冲区 9 字节（包括 '\0'），意味着最多显示 8 个字符
    char str[9];
    // 这里使用 snprintf 将 city 复制到 str 中，防止 city 过长导致溢出
    snprintf(str, 9, "%s", city);

    lcd_show_string(127, 170, str, &font20_maple, 1, BLACK, WHITE);
}

/**
 * 更新室外温度显示（右下区域）
 * 参数：float tempreature - 温度（摄氏度）
 */
void main_page_redraw_outdoor_tempreature(float tempreature)
{
    lcd_fill(135, 186, 189, 240, RGB565(253, 135, 75));

    // 数值缓冲区，默认显示为 "--"
    char str[3] = {'-','-'};

    // 有效范围判断同内置温度
    if(tempreature > -10.0f && tempreature <= 100.0f)
        snprintf(str, sizeof(str), "%2.0f", tempreature);

    lcd_show_string(135,186, str, &font54_maple, 1, BLACK, WHITE);

    lcd_show_string(192, 202, "℃", &font32_maple, 1, BLACK, WHITE);    
}

/**
 * 根据天气代码 code 绘制天气图标
 * 参数：int code - 一个代表天气类型的代码（由天气 API/预先定义）
 * 说明：
 *    - 首先显示一个温度计或默认图标（img_icon_wenduji）在某个位置（133,239）
 *    - 然后根据 code 映射到不同的天气图标（sunny、cloudy、rain 等）
 *    - 如果 code 不匹配任何已知值则使用 img_icon_na（表示不可用/未知）
 */
void main_page_redraw_outdoor_weather_icon(const int code)
{
    lcd_show_image(133, 239, &img_icon_wenduji);

    const image_t *icon = NULL;

    // 根据 code 选择合适的图标
    if(code == 0 || code == 2 || code == 38)
        icon = &img_icon_sunny;
    else if(code == 1 || code == 3)
        icon = &img_icon_moon;
    else if(code == 4 || code == 9)
        icon = &img_icon_yintian;
    else if(code == 5 || code == 6 || code == 7 || code == 8) 
        icon = &img_icon_cloudy;
    else if(code == 10 || code == 13 || code == 14 || code == 15 || code == 16  || code == 17 || code == 18 || code == 19) 
        icon = &img_icon_rain;
    else if(code == 11 || code == 12) 
        icon = &img_icon_leizhenyu;
    else if(code == 20 || code == 21 || code == 22 || code == 23 || code == 24  || code == 25) 
        icon = &img_icon_snow;
    else
        icon = &img_icon_na;

    lcd_show_image(160, 240, icon);
}
