#include "image.h"
#include "lcd.h"
#include "fonts.h"
#include "app.h"
#include <string.h>

void wifi_page_display(void)
{
    static const char *ssid = WIFI_SSID;
    uint16_t start_x = 0;
    int len = strlen(ssid) * 16;
    if(len < lcddev.width)
    {
        start_x = (lcddev.width - len + 1) / 2;
    }

    lcd_fill(0, 0, 240, 320, BLACK);
    lcd_show_image(30, 25, &img_wifi);
    lcd_show_string(88, 200, "WiFi", &font32_maple, 1, GREEN, BLACK);
    lcd_show_string(start_x, 240, ssid, &font32_maple, 1, WHITE, BLACK);
    lcd_show_string(72,280,"������...", &font32_maple, 1,YELLOW, BLACK);
}
