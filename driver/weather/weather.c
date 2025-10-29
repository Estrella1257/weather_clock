#include "weather.h"

//解析心知天气返回的 JSON 格式天气数据
//response  从网络获取的心知天气 HTTP 响应内容（JSON字符串）
//info      存放解析结果的 weather_info_t 结构体指针
bool parse_seniverse_response(const char *response,weather_info_t *info)      
{
    // 找到 JSON 中 "results" 字段的起始位置
    // 如果找不到，说明不是合法的心知天气响应，返回 false
    response = strstr(response,"\"results\":");
    if(response == NULL)
        return false;
    
    // 找到 "location" 字段（包含城市名、地区路径等信息）
    const char *location_response = strstr(response,"\"location\":");
    if(location_response == NULL)
        return false;
    
    // 查找 "location" 中的 "name" 字段（城市名）
    const char *location_name_response = strstr(location_response,"\"name\":");
    if(location_name_response)
    {
        // 读取城市名字符串，限制最大长度为31个字符
        // "%31[^\"]" 表示读取直到下一个双引号为止的31个字符
        sscanf(location_name_response,"\"name\":\"%31[^\"]\"",info->city);
    }

    // 查找 "location" 中的 "path" 字段（地理路径，例如 “中国,广东,深圳”）
    const char *location_path_response = strstr(location_response,"\"path\":");
    if(location_path_response)
    {
        // 读取路径字符串，最多127个字符
        sscanf(location_path_response,"\"path\":\"%127[^\"]\"",info->location);
    }

    // 找到 "now" 字段（当前天气信息）
    const char *now_response = strstr(response,"\"now\":");
    if(now_response == NULL)
        return false;    

    // 查找天气描述字段 "text"（例如 “晴”、“多云”、“小雨”）
    const char *now_text_response = strstr(now_response,"\"text\":");
    if(now_text_response)
    {
        // 读取天气文字描述，最大15个字符
        sscanf(now_text_response,"\"text\":\"%15[^\"]\"",info->weather);
    }

    // 查找天气代码字段 "code"（数字形式的天气代码，用于图标或天气判断）
    const char *now_code_response = strstr(now_response,"\"code\":");
    if(now_code_response)
    {
        // 直接读取整数形式的天气代码
        sscanf(now_code_response,"\"code\":\"%d\"", &info->weather_code);
    }

    // 临时字符串用于存放温度值（心知返回的温度是字符串形式）
    char tempreture_str[16] = { 0 };
    // 查找温度字段 "temperature"
    const char *now_temperature_response = strstr(now_response,"\"temperature\":");
    if(now_temperature_response)
    {
        // 先提取温度的字符串形式（例如 "25"）
        sscanf(now_temperature_response,"\"temperature\":\"%15[^\"]\"",tempreture_str);
        // 转换为浮点数存入结构体
        info->temperature = atof(tempreture_str);
    }

    // 所有字段成功解析则返回 true
    return true;
}