#include "image.h"
#include "lcd.h"
#include "fonts.h"

void welcome_page_display(void)
{
    lcd_fill(0, 0, 240, 320, CYAN);
    lcd_show_image(30, 30, &img_mingrixiang);
    lcd_show_string(52, 216, "Estrella", &font32_maple, 1, GREEN, BLACK);
    lcd_show_string(52,244,"¡¨Ω”÷–...", &font32_maple, 1,YELLOW,BLACK);
    lcd_show_string(43, 272, "Loading...", &font32_maple, 1, BLUE, BLACK);
}
