#include "ui.h"
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

    ui_fill_color(0, 0, 240, 320, BLACK);
    ui_draw_image(30, 25, &img_wifi);
    ui_write_string(88, 200, "WiFi", &font32_maple, 1, GREEN, BLACK);
    ui_write_string(start_x, 240, ssid, &font32_maple, 1, WHITE, BLACK);
    ui_write_string(72,280,"Á¬½ÓÖÐ...", &font32_maple, 1,YELLOW, BLACK);
}
