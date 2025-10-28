#include "rtc.h"
#include "stm32f4xx.h"
#include <string.h>

void rtc_init(void)
{
    RCC_RTCCLKCmd(ENABLE);    // 使能RTC时钟
    RTC_WaitForSynchro();     // 等待RTC寄存器同步完成

    RTC_InitTypeDef RTC_InitStruct;
    RTC_StructInit(&RTC_InitStruct);     // 初始化结构体为默认配置
    RTC_Init(&RTC_InitStruct);      // 应用初始化配置
}

static void rtc_get_time_once(rtc_datetime_t *date_time)
{
    RTC_DateTypeDef date;
    RTC_TimeTypeDef time;
    RTC_DateStructInit(&date);       // 日期结构体清零
    RTC_TimeStructInit(&time);       // 时间结构体清零

    RTC_GetDate(RTC_Format_BIN,&date);     // 获取日期（BIN格式）
    RTC_GetTime(RTC_Format_BIN,&time);     // 获取时间（BIN格式）

    // 将RTC寄存器数据拷贝到用户结构体
    date_time->year = 2000 + date.RTC_Year;    // 年份寄存器以2000为基准
    date_time->month = date.RTC_Month;
    date_time->day = date.RTC_Date;
    date_time->weekday = date.RTC_WeekDay;
    date_time->hour = time.RTC_Hours;
    date_time->minute = time.RTC_Minutes;
    date_time->second = time.RTC_Seconds;
}

void rtc_get_time(rtc_datetime_t *date_time)
{
    rtc_datetime_t time1,time2;

    do
    {
        rtc_get_time_once(&time1);
        rtc_get_time_once(&time2);
    } while (memcmp(&time1,&time2,sizeof(rtc_datetime_t)) != 0);     // 两次读取不一致则重读
    

    memcpy(date_time,&time1,sizeof(rtc_datetime_t));      // 拷贝最终一致的时间数据
}

static void rtc_set_time_once(const rtc_datetime_t *date_time)
{
    RTC_DateTypeDef date;
    RTC_TimeTypeDef time;
    RTC_DateStructInit(&date);     // 日期结构体初始化
    RTC_TimeStructInit(&time);     // 时间结构体初始化

    date.RTC_Year = date_time->year - 2000;     // RTC只保存2000~2099之间的年份
    date.RTC_Month = date_time->month;
    date.RTC_Date = date_time->day;
    date.RTC_WeekDay = date_time->weekday;
    time.RTC_Hours = date_time->hour;
    time.RTC_Minutes = date_time->minute;
    time.RTC_Seconds = date_time->second;

    RTC_SetDate(RTC_Format_BIN,&date);     // 写入日期寄存器
    RTC_SetTime(RTC_Format_BIN,&time);     // 写入时间寄存器
}

void rtc_set_time(const rtc_datetime_t *date_time)
{
    rtc_datetime_t rtime;
    do
    {
        rtc_set_time_once(date_time);      // 写入目标时间
        rtc_get_time_once(&rtime);        // 立即读取以验证
    } while (memcmp(date_time,&rtime,sizeof(rtc_datetime_t)) != 0);      // 若不匹配则重写
    
}