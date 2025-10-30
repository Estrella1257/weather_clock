#include <stdbool.h>    
#include <stdint.h>
#include <string.h>
#include "main.h"
#include "app.h"
#include "pages.h"
#include "esp_at.h"
#include "weather.h"

//时间单位宏定义，方便后续设置延时
#define MILLISECONDS(x) (x)                   // 毫秒直接返回
#define SECONDS(x)		MILLISECONDS((x) * 1000)   // 秒转毫秒
#define MINUTES(x)		SECONDS((x) * 60)          // 分钟转毫秒
#define HOURS(x)		MINUTES((x) * 60)          // 小时转毫秒
#define DAYS(x)			HOURS((x) * 24)           // 天转毫秒

//各任务更新间隔定义
#define TIME_SYNC_INTERVAL			MINUTES(1)    // 网络时间同步间隔 1 分钟
#define WIFI_UPDATE_INTERVAL		SECONDS(5)    // WiFi 状态更新间隔 5 秒
#define TIME_UPDATE_INTERVAL		SECONDS(1)    // RTC 时间更新间隔 1 秒
#define INNER_UPDATE_INTERVAL		SECONDS(5)    // 内部温湿度更新间隔 5 秒
#define OUTDOOR_UPDATE_INTERVAL		SECONDS(5)    // 户外天气更新间隔 5 秒

//各任务延时计数器，单位毫秒
static volatile uint32_t time_sync_delay = 0;      // 网络时间同步延时
static volatile uint32_t wifi_update_delay = 0;   // WiFi 更新延时
static volatile uint32_t time_update_delay = 0;   // 时间显示更新延时
static volatile uint32_t inner_update_delay = 0;  // 内部温湿度更新延时
static volatile uint32_t outdoor_update_delay = 0;// 户外天气更新延时

// 定时器周期回调函数，每 1ms 调用一次
// 用于减少各任务延时计数器
static void tim_periodic_callback(void)
{
	if(time_sync_delay > 0)
		time_sync_delay--;
	if(wifi_update_delay > 0)
		wifi_update_delay--;
	if(time_update_delay > 0)
		time_update_delay--;
	if(inner_update_delay > 0)
		inner_update_delay--;
	if(outdoor_update_delay > 0)
		outdoor_update_delay--;
}

// 网络时间同步函数
// 使用 ESP8266 AT 指令获取 SNTP 时间
static void time_sync(void)
{
	if(time_sync_delay > 0)      // 延时未到，直接返回
		return;
	
	time_sync_delay = TIME_SYNC_INTERVAL;      // 重置延时计数器

	esp_at_datetime_t esp_date = { 0 };        // 用于保存 SNTP 时间结构体
	if(!esp_at_sntp_get_time(&esp_date))       // 获取时间失败
	{
		printf("[SNTP] get time failed\n");
		time_sync_delay = SECONDS(5);           // 5秒后重试
		return;
	}

	if(esp_date.year < 2000)        // 获取到的时间不合法
	{
		printf("[SNTP] invalid date formate\n");
		time_sync_delay = SECONDS(5);
		return;
	}

	printf("[SNTP] sync time:%4u-%02u-%02u %02u:%02u:%02u (%d)\n",
		esp_date.year,esp_date.month,esp_date.day,esp_date.hour,esp_date.minute,esp_date.second,esp_date.weekday);
	
	// 将 SNTP 时间写入 RTC
	rtc_datetime_t rtc_date = { 0 };
	rtc_date.year = esp_date.year;
	rtc_date.month = esp_date.month;
	rtc_date.day = esp_date.day;
	rtc_date.hour = esp_date.hour;
	rtc_date.minute = esp_date.minute;
	rtc_date.second = esp_date.second;
	rtc_date.weekday = esp_date.weekday;
	rtc_set_time(&rtc_date);         
	
	time_update_delay = 10;           // 同步时间后立即更新显示
}

//WiFi 状态刷新函数
static void wifi_update(void)
{
	static esp_at_wifi_info_t last_info = { 0 };     // 上一次 WiFi 信息，方便比较是否变化

	if(wifi_update_delay > 0)
		return;
	
	wifi_update_delay = WIFI_UPDATE_INTERVAL;

	esp_at_wifi_info_t info = { 0 };       // 当前 WiFi 信息
	if(!esp_at_get_wifi_info(&info))      
	{
		printf("[AT] wifi info get failed\n"); 
		main_page_redraw_wifi_ssid("wifi lost");     // 显示丢失状态
		last_info.connected = 0;                     // 标记未连接     
		return;
	}

	// WiFi 已连接
	if(info.connected)
	{
		// 连接状态或信息变化才刷新显示
		if(last_info.connected != info.connected || memcmp(&info, &last_info, sizeof(info)) != 0)
		{
			main_page_redraw_wifi_ssid(info.ssid);      // 更新页面显示 SSID
		}
	}
	else
	{
		main_page_redraw_wifi_ssid("wifi lost");   
	}

	// 保存当前 WiFi 信息供下一次比较
	memcpy(&last_info,&info,sizeof(esp_at_wifi_info_t));
}

// RTC 时间更新函数
static void time_update(void)
{
	static rtc_datetime_t last_date = { 0 };      // 上一次时间显示

	if(time_update_delay > 0)
	return;
	
	time_update_delay = TIME_UPDATE_INTERVAL;

	rtc_datetime_t date;
	rtc_get_time(&date);         // 读取 RTC 时间
    
	// 时间（时分秒）变化才刷新显示
	if(last_date.hour != date.hour || last_date.minute != date.minute || last_date.second != date.second)
	{
		main_page_redraw_time(&date);
	}

	// 日期（年月日）变化才刷新显示
	if(last_date.year != date.year || last_date.month != date.month || last_date.day != date.day || last_date.weekday != date.weekday)
	{
		main_page_redraw_date(&date);
	}

	// 保存当前时间供下一次比较
	memcpy(&last_date,&date,sizeof(rtc_datetime_t));
}

// 内部温湿度更新函数
static void inner_update(void)
{
	static float last_temperature, last_humidity;

	if(inner_update_delay > 0)
		return;
	
	inner_update_delay = INNER_UPDATE_INTERVAL;

	// 开始测量
	if(!aht20_start_measurement())
	{
		printf("[AHT20] start measurement failed\n");
		return;
	}

	// 等待测量完成
	if(!aht20_wait_for_measurement())
	{
		printf("[AHT20] wait for measurement failed\n");
		return;
	}

	float temperature = 0.0f, humidity = 0.0f;

	if(!aht20_read_measurement(&temperature,&humidity))
	{
		printf("[AHT20] read measurement failed\n");
		return;
	}

	// 温度未变化且湿度未变化时不刷新
	if(last_temperature != temperature && last_humidity == humidity)
	{
		return;
	}

	// 保存新数据
	last_temperature = temperature;
	last_humidity = humidity;

	// 打印并刷新页面显示
	printf("[AHT20] Temperature: %.1f, Humidity: %.1f\n",temperature,humidity);
	main_page_redraw_inner_tempreature(temperature);
	main_page_redraw_inner_humidity(humidity);
}

// 户外天气信息更新函数
static void outdoor_update(void)
{
	static weather_info_t last_weather = { 0 };        // 上一次天气信息
	
	if(outdoor_update_delay > 0)	
		return;

	outdoor_update_delay = OUTDOOR_UPDATE_INTERVAL;

	weather_info_t weather = { 0 };

	const char *weather_url = WIFI_URL;
	const char *weather_http_response = esp_at_http_get(weather_url);        // 获取 HTTP 响应
	if(weather_http_response == NULL)
	{
		printf("[WEATHER] http error\n");
		return;
	}

	if(!parse_seniverse_response(weather_http_response,&weather))
	{
		printf("[WEATHER] parse failed\n");
		return;
	}

	// 数据未变化，不刷新
	if(memcmp(&last_weather,&weather,sizeof(weather_info_t)) == 0)
	{
		return;
	}

	memcpy(&last_weather,&weather,sizeof(weather_info_t));

	// 打印并刷新显示
	printf("[WEATHER] %s, %s, %.1f\n",weather.city,weather.weather,weather.temperature);
	main_page_redraw_outdoor_tempreature(weather.temperature);
	main_page_redraw_outdoor_weather_icon(weather.weather_code);
}

void main_loop_init(void)
{
    tim_register_callback(tim_periodic_callback);
}

void main_loop(void)
{
    time_sync();       // 网络时间同步
    wifi_update();     // WiFi 状态更新
    time_update();     // 时间显示更新
    inner_update();    // 内部温湿度更新
    outdoor_update();  // 户外天气更新
}