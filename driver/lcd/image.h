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


#endif