#ifndef __UI_H__
#define __UI_H__

#include <stdint.h>
#include "fonts.h"
#include "image.h"
#include "lcd.h"

#define UI_WIDTH    240
#define UI_HEIGHT   320

void ui_init(void);
void ui_fill_color(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height, uint16_t color);
void ui_write_string(uint16_t x, uint16_t y, const uint8_t *String, const font_t *font, uint8_t mode, uint16_t color, uint16_t bgcolor);
void ui_draw_image(uint16_t x, uint16_t y, const image_t *img);

#endif 