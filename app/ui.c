#pragma anon_unions
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "lcd.h"
#include "ui.h"

typedef enum
{
    UI_ACTION_FILL_COLOR,
    UI_ACTION_WRITE_STRING,
    UI_ACTION_DRAW_IMAGE,
} ui_action_t;

typedef struct
{
    ui_action_t action;
    union
    {
        struct  // 匹配 lcd_fill()
        {
            uint16_t sx;
            uint16_t sy;
            uint16_t width;
            uint16_t height;
            uint16_t color;
        } fill_color;

        struct  // 匹配 lcd_show_string()
        {
            uint16_t x;
            uint16_t y;
            const font_t *font;
            uint8_t mode;
            uint16_t color;
            uint16_t bgcolor;
            char *str; // 动态复制字符串
        } write_string;

        struct  // 匹配 lcd_show_image()
        {
            uint16_t x;
            uint16_t y;
            const image_t *img;
        } draw_image;
    };
} ui_message_t;

static QueueHandle_t ui_queue = NULL;

static void ui_task(void *param)
{
    ui_message_t msg;

    lcd_init();

    for (;;)
    {
        if (xQueueReceive(ui_queue, &msg, portMAX_DELAY) == pdTRUE)
        {
            switch (msg.action)
            {
            case UI_ACTION_FILL_COLOR:
                lcd_fill(
                    msg.fill_color.sx,
                    msg.fill_color.sy,
                    msg.fill_color.width,
                    msg.fill_color.height,
                    msg.fill_color.color);
                break;

            case UI_ACTION_WRITE_STRING:
                lcd_show_string(
                    msg.write_string.x,
                    msg.write_string.y,
                    (const uint8_t *)msg.write_string.str,
                    msg.write_string.font,
                    msg.write_string.mode,
                    msg.write_string.color,
                    msg.write_string.bgcolor);
                vPortFree(msg.write_string.str);
                break;

            case UI_ACTION_DRAW_IMAGE:
                lcd_show_image(
                    msg.draw_image.x,
                    msg.draw_image.y,
                    msg.draw_image.img);
                break;

            default:
                printf("Unknown UI action: %d\n", msg.action);
                break;
            }
        }
    }
}

void ui_init(void)
{
    ui_queue = xQueueCreate(64, sizeof(ui_message_t));
    configASSERT(ui_queue);
    xTaskCreate(ui_task, "ui_task", 2048, NULL, 8, NULL);
}

void ui_fill_color(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height, uint16_t color)
{
    if (ui_queue == NULL) return;

    ui_message_t msg = {0};
    msg.action = UI_ACTION_FILL_COLOR;
    msg.fill_color.sx = sx;
    msg.fill_color.sy = sy;
    msg.fill_color.width = width;
    msg.fill_color.height = height;
    msg.fill_color.color = color;
    xQueueSend(ui_queue, &msg, portMAX_DELAY);
}

void ui_write_string(uint16_t x, uint16_t y, const uint8_t *String, const font_t *font,
                     uint8_t mode, uint16_t color, uint16_t bgcolor)
{
    if (ui_queue == NULL) return;

    ui_message_t msg = {0};
    msg.action = UI_ACTION_WRITE_STRING;
    msg.write_string.x = x;
    msg.write_string.y = y;
    msg.write_string.font = font;
    msg.write_string.mode = mode;
    msg.write_string.color = color;
    msg.write_string.bgcolor = bgcolor;

    size_t len = strlen((const char *)String) + 1;
    msg.write_string.str = pvPortMalloc(len);
    if (msg.write_string.str == NULL)
    {
        printf("[UI] malloc failed for string: %s\n", String);
        return;
    }
    strcpy(msg.write_string.str, (const char *)String);

    xQueueSend(ui_queue, &msg, portMAX_DELAY);
}

void ui_draw_image(uint16_t x, uint16_t y, const image_t *img)
{
    if (ui_queue == NULL) return;

    ui_message_t msg = {0};
    msg.action = UI_ACTION_DRAW_IMAGE;
    msg.draw_image.x = x;
    msg.draw_image.y = y;
    msg.draw_image.img = img;
    xQueueSend(ui_queue, &msg, portMAX_DELAY);
}
