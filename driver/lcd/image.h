#ifndef __IMAGE_H__
#define __IMAGE_H__

#include <stdint.h>

typedef struct
{
    uint16_t width;      // 图像宽度（像素）
    uint16_t height;     // 图像高度（像素）
    const uint8_t *data; // 图像数据指针（RGB565格式）
} image_t;

extern const image_t img_mingrixiang;
extern const image_t img_error;
extern const image_t img_wifi;
extern const image_t img_icon_cloudy;
extern const image_t img_icon_leizhenyu;
extern const image_t img_icon_moon;
extern const image_t img_icon_na;
extern const image_t img_icon_rain;
extern const image_t img_icon_snow;
extern const image_t img_icon_sunny;
extern const image_t img_icon_yintian;
extern const image_t img_icon_wifi;
extern const image_t img_icon_wenduji;

#endif