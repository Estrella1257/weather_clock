#ifndef __LCD_H__
#define __LCD_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "stm32f4xx.h"
#include "fonts.h"
#include "image.h"

#define u8   uint8_t
#define u16  uint16_t
#define u32  uint32_t

typedef struct 
{
    uint16_t width;     // LCD宽度（像素数）
    uint16_t height;    // LCD高度（像素数）
    uint16_t id;        // LCD控制器芯片ID
    uint8_t direction;        // 屏幕方向：0=竖屏，1=横屏 // 屏幕方向：0=竖屏，1=横屏 
    uint16_t wramcmd;   // 写GRAM（显存）的指令
    uint16_t setxcmd;   // 设置X坐标的指令
    uint16_t setycmd;   // 设置X坐标的指令
} lcd_dev_t;

extern lcd_dev_t lcddev;
extern uint16_t POINT_COLOR;
extern uint16_t BACK_COLOR;

// LCD地址结构体，用于FSMC外设访问LCD
typedef struct 
{
    uint16_t LCD_REG;
    uint16_t LCD_RAM;
} LCD_TypeDef;

// LCD 基地址：使用STM32 FSMC 外部存储控制器
#define LCD_BASE        ((uint32_t)(0x60000000 | 0x00007FFFE))
#define LCD             ((LCD_TypeDef *) LCD_BASE)

//扫描方向定义
#define L2R_U2D  0 // 从左到右，从上到下
#define L2R_D2U  1 // 从左到右，从下到上
#define R2L_U2D  2 // 从右到左，从上到下
#define R2L_D2U  3 // 从右到左，从下到上

#define U2D_L2R  4 // 从上到下，从左到右
#define U2D_R2L  5 // 从上到下，从右到左
#define D2U_L2R  6 // 从下到上，从左到右
#define D2U_R2L  7 // 从下到上，从右到左     

#define DFT_SCAN_DIR  L2R_U2D  // 默认扫描方向

// RGB565 常用颜色定义
#define BLACK       0x0000  // 黑色
#define WHITE       0xFFFF  // 白色

#define RED         0xF800  // 红色
#define GREEN       0x07E0  // 绿色
#define BLUE        0x001F  // 蓝色

#define YELLOW      0xFFE0  // 黄色 (红+绿)
#define CYAN        0x07FF  // 青色 (绿+蓝)
#define MAGENTA     0xF81F  // 品红 (红+蓝)

#define GRAY        0x8430  // 灰色
#define DARKGRAY    0x4208  // 深灰
#define LIGHTGRAY   0xC618  // 浅灰

#define ORANGE      0xFC00  // 橙色
#define BROWN       0x8200  // 棕色
#define PINK        0xF81F  // 粉色 (跟品红接近)
#define PURPLE      0x780F  // 紫色

#define NAVY        0x000F  // 藏青
#define TEAL        0x0410  // 蓝绿
#define OLIVE       0x8400  // 橄榄绿

#define GOLD        0xFEA0  // 金色
#define SILVER      0xC618  // 银色 (近似)


// 界面常用的扩展颜色
#define DARKBLUE   0X01CF  // 深蓝色
#define LIGHTBLUE  0X7D7C  // 浅蓝色  
#define GRAYBLUE   0X5458  // 灰蓝色

#define LIGHTGREEN 0X841F  // 浅绿色
#define LGRAY      0XC618  // 浅灰色（窗体背景）
#define LGRAYBLUE  0XA651  // 浅灰蓝色
#define LBBLUE     0X2B12  // 浅棕蓝色

#define ORANGERED   0xFA20  // 橙红色


//LCD 驱动函数声明
void lcd_init(void);                                  // 初始化LCD
void lcd_display_on(void);                             // 打开显示
void lcd_display_off(void);                            // 关闭显示
void lcd_clear(u16 color);                            // 清屏
void lcd_set_cursor(u16 x_pos, u16 y_pos);            // 设置光标
void lcd_draw_point(u16 x,u16 y,u16 color);           //画点 
void lcd_fast_draw_point(u16 x,u16 y,u16 color);
void lcd_fill(u16 sx,u16 sy,u16 width,u16 height,u16 color); // 区域填充单色

// 显示字符/字符串
void lcd_show_char(u16 x, u16 y, u8 ch, const font_t *font, u8 mode, u16 color, u16 bgcolor);
void lcd_show_string(u16 x, u16 y, const u8 *String, const font_t *font, u8 mode, u16 color, u16 bgcolor);
void lcd_show_chinese_char(u16 x, u16 y, const char *name, const font_t *font, u8 mode, u16 color, u16 bgcolor);
void lcd_show_image(u16 x, u16 y, const image_t *img);

// 读写寄存器
void lcd_write_register(vu16 regaddress, vu16 data);
u16 lcd_read_register(vu16 regaddress);
void lcd_write_ram_prepare(void);
void lcd_write_ram(u16 RGB_code);
u16 RGB565(u8 r, u8 g, u8 b);

// 控制函数
void lcd_scan_dir(u8 direction);                 // 设置扫描方向
void lcd_display_dir(u8 direction);             // 设置屏幕方向（横/竖屏）



#endif