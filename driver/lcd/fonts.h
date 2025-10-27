#ifndef __FONTS_H__
#define __FONTS_H__

#include <stdint.h>


typedef struct
{
    const char *name;
    const uint8_t *model;
} font_chinese_t;

// ASCII 字库结构体
typedef struct
{
    const uint8_t *ascii_model;   // ASCII 字模表指针
    const char *ascii_map;        // ASCII 字符映射表（如 "0123456789:- "）
    const font_chinese_t *chinese;// 中文字库（暂未使用）
    uint16_t size;                // 字体高度（像素）
    uint8_t ascii_count;          // 可显示字符数量
} font_t;

extern const font_t font16_maple;
extern const font_t font20_maple;
extern const font_t font24_maple;
extern const font_t font32_maple;
extern const font_t font54_maple;
extern const font_t font62_maple;
extern const font_t font76_maple;

#endif