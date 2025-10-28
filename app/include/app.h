#ifndef __APP_H__
#define __APP_H__

#include <stdint.h>
#include <stdbool.h>

#define APP_VERSION "v1.0"
#define WIFI_SSID "Redmi K60 Pro"
#define WIFI_PASSWORD "12345678"
#define WIFI_URL "https://api.seniverse.com/v3/weather/now.json?key=SU8PEBzZMEJ22jpWz&location=zhanjiang&language=en&unit=c"

void wifi_init(void);
void wifi_wait_connected(void);

void main_loop_init(void);


#endif