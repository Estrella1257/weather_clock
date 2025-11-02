#include <stdbool.h>    
#include <stdint.h>
#include <string.h>
#include "main.h"
#include "app.h"
#include "pages.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "esp_at.h"
#include "aht20.h"
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

// // 定时器周期回调函数，每 1ms 调用一次
// // 用于减少各任务延时计数器
// static void tim_periodic_callback(void)
// {
// 	if(time_sync_delay > 0)
// 		time_sync_delay--;
// 	if(wifi_update_delay > 0)
// 		wifi_update_delay--;
// 	if(time_update_delay > 0)
// 		time_update_delay--;
// 	if(inner_update_delay > 0)
// 		inner_update_delay--;
// 	if(outdoor_update_delay > 0)
// 		outdoor_update_delay--;
// }

#define LOOP_EVT_TIME_SYNC         (1 << 0)
#define LOOP_EVT_WIFI_UPDATE       (1 << 1)
#define LOOP_EVT_TIME_UPDATE       (1 << 2)
#define LOOP_EVT_INNER_UPDATE      (1 << 3)
#define LOOP_EVT_OUTDOOR_UPDATE    (1 << 4)
#define LOOP_EVT_ALL               (LOOP_EVT_TIME_SYNC     | \
                                    LOOP_EVT_WIFI_UPDATE   | \
                                    LOOP_EVT_TIME_UPDATE   | \
                                    LOOP_EVT_INNER_UPDATE  | \
                                    LOOP_EVT_OUTDOOR_UPDATE)

static TaskHandle_t loop_task;
static TimerHandle_t time_sync_timer;
static TimerHandle_t wifi_update_timer;
static TimerHandle_t time_update_timer;
static TimerHandle_t inner_update_timer;
static TimerHandle_t outdoor_update_timer;

// 网络时间同步函数
// 使用 ESP8266 AT 指令获取 SNTP 时间
static void time_sync(void)
{
    uint32_t restart_sync_delay = SECONDS(5); // 失败后每5秒重试，成功后改为 TIME_SYNC_INTERVAL

    rtc_datetime_t rtc_date = { 0 };
    esp_at_datetime_t esp_date = { 0 };

    // 先检查当前 WIFI 是否连上（避免太早请求 SNTP）
    esp_at_wifi_info_t wifi_info = { 0 };
    if (!esp_at_get_wifi_info(&wifi_info) || !wifi_info.connected)
    {
        printf("[SNTP] wifi not connected yet, will retry in %u s\n", restart_sync_delay / 1000);
        // 以定时器方式重试（不立刻通知）
        xTimerChangePeriod(time_sync_timer, pdMS_TO_TICKS(restart_sync_delay), 0);
        return;
    }

    // 请求 SNTP 时间
    if (!esp_at_sntp_get_time(&esp_date))
    {
        printf("[SNTP] get time failed, retry in %u s\n", restart_sync_delay / 1000);
        xTimerChangePeriod(time_sync_timer, pdMS_TO_TICKS(restart_sync_delay), 0);
        return;
    }

    if (esp_date.year <= 2000)
    {
        printf("[SNTP] invalid date format (year=%u), retry in %u s\n", esp_date.year, restart_sync_delay / 1000);
        xTimerChangePeriod(time_sync_timer, pdMS_TO_TICKS(restart_sync_delay), 0);
        return;
    }

    // 成功 —— 设置 RTC，并把下一次同步设为正常间隔
    printf("[SNTP] sync time:%4u-%02u-%02u %02u:%02u:%02u (%d)\n",
           esp_date.year, esp_date.month, esp_date.day,
           esp_date.hour, esp_date.minute, esp_date.second, esp_date.weekday);

    rtc_date.year = esp_date.year;
    rtc_date.month = esp_date.month;
    rtc_date.day = esp_date.day;
    rtc_date.hour = esp_date.hour;
    rtc_date.minute = esp_date.minute;
    rtc_date.second = esp_date.second;
    rtc_date.weekday = esp_date.weekday;
    rtc_set_time(&rtc_date);

    // 成功后把定时器周期恢复为正常的每小时同步
    xTimerChangePeriod(time_sync_timer, pdMS_TO_TICKS(TIME_SYNC_INTERVAL), 0);
}

//WiFi 状态刷新函数
static void wifi_update(void)
{
    static esp_at_wifi_info_t last_info = { 0 };

    xTimerChangePeriod(wifi_update_timer, pdMS_TO_TICKS(WIFI_UPDATE_INTERVAL), 0);

    esp_at_wifi_info_t info = { 0 };
    if(!esp_at_get_wifi_info(&info))
    {
        printf("[AT] wifi info get failed\n");
        main_page_redraw_wifi_ssid("wifi lost");     // 显示丢失状态
        last_info.connected = 0;                    // 标记未连接
        return;
    }

    if(info.connected)
    {
        // 当从未连接 -> 已连接时，立即触发一次 SNTP（但 SNTP 函数内部也会再验证 wifi）
        if(last_info.connected == 0)
        {
            printf("[WiFi] Connected, trigger SNTP sync\n");
            // 直接启动一次 time_sync_timer，让 time_sync 在定时器回调执行（避免直接在本函数中做网络延时操作）
            xTimerChangePeriod(time_sync_timer, pdMS_TO_TICKS(1), 0); // 1 ms 后触发一次立即尝试
        }

        if(last_info.connected != info.connected || memcmp(&info, &last_info, sizeof(info)) != 0)
        {
            main_page_redraw_wifi_ssid(info.ssid);     // 更新页面显示 SSID
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
	static rtc_datetime_t last_date = { 0 };        // 上一次时间显示

	xTimerChangePeriod(time_update_timer, pdMS_TO_TICKS(TIME_UPDATE_INTERVAL), 0);

	rtc_datetime_t date;
	rtc_get_time(&date);       // 读取 RTC 时间

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

	xTimerChangePeriod(inner_update_timer, pdMS_TO_TICKS(INNER_UPDATE_INTERVAL), 0);

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
	static weather_info_t last_weather = { 0 };

	if (xTimerIsTimerActive(outdoor_update_timer) == pdFALSE) 
	{
        xTimerChangePeriod(outdoor_update_timer, pdMS_TO_TICKS(OUTDOOR_UPDATE_INTERVAL), 0);
    }

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

static void loop_func(void *param)
{
	uint32_t event;

	while(1)
	{
		event = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		if(event & LOOP_EVT_TIME_SYNC)
		{
			time_sync();
		}
		if(event & LOOP_EVT_WIFI_UPDATE)
		{
			wifi_update();
		}
		if(event & LOOP_EVT_TIME_UPDATE)
		{
			time_update();
		}
		if(event & LOOP_EVT_INNER_UPDATE)
		{
			inner_update();
		}
		if(event & LOOP_EVT_OUTDOOR_UPDATE)
		{
			outdoor_update();
		}
	}
}

static void loop_timer_cb(TimerHandle_t timer)
{
	uint32_t event = (uint32_t)pvTimerGetTimerID(timer);
	xTaskNotify(loop_task, event, eSetBits);
}

void main_loop_init(void)
{
	time_sync_timer = xTimerCreate("time sync", 1, pdFALSE, (void *)LOOP_EVT_TIME_SYNC, loop_timer_cb);
	wifi_update_timer = xTimerCreate("wifi update", pdMS_TO_TICKS(WIFI_UPDATE_INTERVAL), pdTRUE, (void *)LOOP_EVT_WIFI_UPDATE, loop_timer_cb);
	time_update_timer = xTimerCreate("time update", pdMS_TO_TICKS(TIME_UPDATE_INTERVAL), pdTRUE, (void *)LOOP_EVT_TIME_UPDATE, loop_timer_cb);
	inner_update_timer = xTimerCreate("inner update", pdMS_TO_TICKS(INNER_UPDATE_INTERVAL), pdTRUE, (void *)LOOP_EVT_INNER_UPDATE, loop_timer_cb);
	outdoor_update_timer = xTimerCreate("outdoor update", pdMS_TO_TICKS(OUTDOOR_UPDATE_INTERVAL), pdTRUE, (void *)LOOP_EVT_OUTDOOR_UPDATE, loop_timer_cb);

	xTaskCreate(loop_func, "loop", 1024, NULL, 5, &loop_task);

	xTaskNotify(loop_task, LOOP_EVT_ALL, eSetBits);

	xTimerStart(wifi_update_timer, 0);
	xTimerStart(time_update_timer, 0);
	xTimerStart(inner_update_timer, 0);
	xTimerStart(outdoor_update_timer, 0);
}