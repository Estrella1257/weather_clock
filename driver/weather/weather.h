#ifndef __WEATHER_H__
#define __WEATHER_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct
{
    char city[32];
    char location[128];
    char weather[16];
    int weather_code;
    float temperature;
} weather_info_t;

bool parse_seniverse_response(const char *response,weather_info_t *info);

#endif