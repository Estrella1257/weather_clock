#include "ui.h"

void welcome_page_display(void)
{
    ui_fill_color(0, 0, 240, 320, CYAN);
    ui_draw_image(30, 30, &img_mingrixiang);
    ui_write_string(52, 216, "Estrella", &font32_maple, 1, GREEN, BLACK);
    ui_write_string(52,244,"¡¨Ω”÷–...", &font32_maple, 1,YELLOW,BLACK);
    ui_write_string(43, 272, "Loading...", &font32_maple, 1, BLUE, BLACK);
}
