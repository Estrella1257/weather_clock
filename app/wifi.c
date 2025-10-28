#include "esp_at.h"
#include <stdio.h>
#include "pages.h"
#include "app.h"
#include "delay.h"

void wifi_init(void)
{
	printf("[SYS] Build Date: %s %s\n", __DATE__, __TIME__);
	
    if(!esp_at_init())
	{
		printf("[AT] init failed!\n");
		goto err;
	}
	printf("[AT] inited!\n");
	
	if (!esp_at_wifi_init())
	{
		printf("[WIFI] init failed!\n");
		goto err;
	}
	printf("[WIFI] inited!\n");	

	if (!esp_at_connect_wifi("Redmi K60 Pro","12345678", NULL))
	{
		printf("[WIFI] connect failed!\n");
		goto err;
	}
	printf("[WIFI] connecting...\n");

	if (!esp_at_sntp_init())
	{
		printf("[SNTP] init failed!\n");
		goto err;
	}
	printf("[SNTP] inited!\n");

	return;

err:
    error_page_display("WiFi Init Failed");
    while(1)
    {

    }
}

void wifi_wait_connected(void)
{
    printf("[WIFI] connecting...\n");

    esp_at_connect_wifi(WIFI_SSID,WIFI_PASSWORD, NULL);

    for (uint32_t t = 0;t < 10 * 1000;t += 100)
    {
        delay_ms(100);
        esp_at_wifi_info_t wifi = { 0 };
		if(esp_at_get_wifi_info(&wifi) && wifi.connected)
        {
            printf("[WIFI] connected\n");
            printf("[WIFI] SSID: %s, BSSID: %s, Channel: %d, RSSI: %d\n",wifi.ssid,wifi.bssid,wifi.channel,wifi.rssi);
            return;
        }
        else if(t % 1000 == 0)
		{
			printf("[AT] wifi info get failed\n");
			continue;
		}
    }
    
    printf("[WIFI] connect timeout\n");
    error_page_display("WiFi Connect Timeout");
    while(1)
    {

    }
}