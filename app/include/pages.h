#ifndef __PAGES_H__
#define __PAGES_H__

#include "rtc.h"

void welcome_page_display(void);
void error_page_display(const char *msg);
void wifi_page_display(void);
void main_page_display(void);
void main_page_redraw_wifi_ssid(const char *ssid);
void main_page_redraw_time(rtc_datetime_t *time);
void main_page_redraw_date(rtc_datetime_t *date);
void main_page_redraw_inner_tempreature(float tempreature);
void main_page_redraw_inner_humidity(float humiduty);
void main_page_redraw_outdoor_city(const char *city);
void main_page_redraw_outdoor_tempreature(float tempreature);
void main_page_redraw_outdoor_weather_icon(const int code);

#endif