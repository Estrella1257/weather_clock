#include "image.h"
#include "lcd.h"
#include "fonts.h"
#include "delay.h"
#include <string.h>

void error_page_display(const char *msg)
{
    lcd_fill(0, 0, 240, 320, BLACK);
    lcd_show_image(40, 40, &img_error);

    uint16_t start_x = 0;
    int len = strlen(msg) * 24 / 2;
    if (len < lcddev.width)
        start_x = (lcddev.width - len + 1) / 2;
    lcd_show_string(start_x, 250, msg, &font24_maple, 1, YELLOW, BLACK);

}