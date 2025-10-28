#include "lcd.h"
#include "rtc.h"
#include "app.h"
#include <string.h>
#include <stdio.h>
#include <pages.h>

// rtc_datetime_t �ṹ�����ڴ�� RTC ��õ�ʱ��������Ϣ��
// ���ļ���������һ��ȫ�ֱ��� now������ main_page_display �������Ӻ�����ȡ��ǰʱ�䡣
// ע�⣺���ϵͳ�Ƕ��߳�/�жϷ��� RTC�����ȫ�ֱ����ķ��ʿ�����Ҫͬ������������/���жϵȣ���
rtc_datetime_t now;  // ��ǰ RTC ʱ�䣨ȫ�ֱ�����

/**
 * ��ҳ������ƣ�һ���Գ�ʼ��/�ػ棩
 * �������������Ƹ���ģ�飨WiFi��ʱ��/���ڡ�������Ϣ��������Ϣ��
 * ˵����do { ... } while (0); �����ﱻ���ڰ�һ����Ʋ�������һ������飨�����۵�/�ֲ�����/�Ӿ����飩��
 */
void main_page_display(void)
{
    lcd_fill(0, 0, 240, 320, BLACK);

    // ������Ϣ���򣨰׵ף�
    do
    {
        lcd_fill(15,15,224,154,WHITE);

        lcd_show_image(23, 20, &img_icon_wifi);

        // �ڱ�������ʾ WiFi SSID��ʹ��ȫ�ֺ�/���� WIFI_SSID��
        main_page_redraw_wifi_ssid(WIFI_SSID);

        // �� RTC ��ȡ��ǰʱ�䲢������ȫ�� now ��
        // rtc_get_time ����Ӧ��� rtc_datetime_t����ȷ�� rtc_get_time ����ǰ RTC ��������Ч�ģ�
        rtc_get_time(&now);

        // ����Ļ���ػ�ʱ�䣨Сʱ:���ӣ�
        main_page_redraw_time(&now);

        // ����Ļ���ػ����ڣ�YYYY/MM/DD ����X��
        main_page_redraw_date(&now);
    }while (0);
    
    // �������ڻ�����Ϣ������ɫ������
    do 
    {
        lcd_fill(15,165,114,304,TEAL);

        lcd_show_string(25, 170, "���ڻ���", &font20_maple, 1, BLACK, WHITE);

        // ��ʼֵΪ "--"����ʾδ֪/δ��ʼ�����������������ַ� '-' ��� char �����ʼ��
        main_page_redraw_inner_tempreature('--');
        main_page_redraw_inner_humidity('--');
    }while(0);
    
    // ����������Ϣ���򣨳�ɫ������
    do
    {
        lcd_fill(125,165,224,304,RGB565(253, 135, 75));

        // ���Ƴ�����
        main_page_redraw_outdoor_city("տ��");

        // �����¶�Ĭ����ʾ "--"
        main_page_redraw_outdoor_tempreature('--');

        // ��ʼ����ͼ��Ϊ -1����ʾδ֪��������ʾĬ�� NA ͼ��
        main_page_redraw_outdoor_weather_icon(-1);
    } while (0);
    
}

/**
 * �� WiFi ������� SSID ��ʾ
 * ������const char *ssid - Ҫ��ʾ�� SSID �ַ���
 */
void main_page_redraw_wifi_ssid(const char *ssid)
{
    lcd_fill(50, 23, 200, 43, WHITE);

    // ��������С 15 �ֽڣ�������β '\0'��
    // ������ snprintf(str,15,"%14s", ssid) �� SSID ���Ƶ���� 14 ���ַ�����֤�����
    // ע����� SSID �� 14 �ָ������ᱻ�ضϣ���ɸ�����Ļ��ȵ�����ֵ
    char str[15];
    snprintf(str, 15, "%14s", ssid);

    lcd_show_string(50, 23, str, &font20_maple, 1, GREEN, WHITE);
}

/**
 * �ڱ������ػ�ʱ�䣨��ʽ HH:MM��
 * ������rtc_datetime_t *time - ָ���� hour��minute ��ʱ��ṹ��
 */
void main_page_redraw_time(rtc_datetime_t *time)
{
    lcd_fill(25, 42, 215, 118, WHITE);

    // ������ 6 �ֽڣ�HH:MM + '\0' ��Ҫ 6 �ֽڣ����� "08:05\0"��
    char str[6];
    // ʹ�� %02u ��֤Сʱ�ͷ���������λ��0 ��䣩������ "09:07"
    // ע�⣺time->hour/time->minute ������Ӧ�� %02u ƥ�䣨ͨ���� unsigned int/uint8_t��
    snprintf(str,sizeof(str),"%02u:%02u",time->hour ,time->minute);

    lcd_show_string(25, 42, str, &font76_maple, 1, BLACK, WHITE);
}

/**
 * �ڱ������ػ����ڣ���ʽ YYYY/MM/DD ����X��
 * ������rtc_datetime_t *date - ���� year,month,day,weekday
 */
void main_page_redraw_date(rtc_datetime_t *date)
{
    lcd_fill(35, 121, 55, 141, WHITE);

    // ������ 18 �ֽڣ����� '\0'����
    // ���� "2025/10/27 ����һ" ����Ϊ 4+1+2+1+2+1+3 = 14����ͬ���Ի����������в�𣩣�18 �ǰ�ȫ��
    char str[18];

    // weekday ��Ӧ���� "һ" �� "��"���� weekday ���� 1-7 ��ʱ����ʾ "X" ��Ϊռλ
    // ������������������������ַ�������
    snprintf(str,sizeof(str),"%04u/%02u/%02u ����%s",date->year,date->month,date->day,
                date-> weekday == 1 ? "һ" :
                date-> weekday == 2 ? "��" :
                date-> weekday == 3 ? "��" :
                date-> weekday == 4 ? "��" :
                date-> weekday == 5 ? "��" :
                date-> weekday == 6 ? "��" :
                date-> weekday == 7 ? "��" : "X" 
            );

    lcd_show_string(35, 121, str, &font20_maple, 1, BLACK, WHITE);
}

/**
 * ���ƻ���������¶���ʾ
 * ������float tempreature - �¶ȣ����϶ȣ�������������Чֵ���� '--' �ı��룩������ʾ "--"
 * ˵���������ڲ��� float Ϊ���룬����ʵ�ʵ���ʱ������� '--'���ַ����������� C �� '--' �ᱻ����Ϊ�������� ���� �������ԭ��������һ��Ǳ�ڲ������㡣
 * ���飺���ʹ������ֵ���� NAN�������������ֵ����ʾδ֪���������ַ�������
 */
void main_page_redraw_inner_tempreature(float tempreature)
{
    lcd_fill(30, 195, 100, 249, TEAL);

    // �ַ��������� 3 �ֽڣ����ڴ����λ�� + '\0'������ "23\0"
    // ��ʼ��Ϊ {'-','-'}���ַ���ĩβ�Զ�����һ�� snprintf д�� '\0'���������ֶ���ʼ��δ���� '\0' ���� ʵ�� C �涨��̬��ʼ������β����0��
    char str[3] = {'-','-'};

    // ��Ч�¶��жϣ��� -10.0 �� 100.0 ���϶�֮�����Ϊ�Ϸ�����ʽ��Ϊ������ʾ
    // ��ѡ���� %2.0f���������뵽��������ռ 2 ���ַ����
    if(tempreature > -10.0f && tempreature <= 100.0f)
        snprintf(str, sizeof(str), "%2.0f", tempreature);

    lcd_show_string(30, 195, str, &font54_maple, 1, BLACK, TEAL);

    lcd_show_string(86, 213, "��", &font32_maple,1, BLACK, WHITE);    
}

/**
 * ���ƻ��������ʪ����ʾ
 * ������float humiduty - ʪ��ֵ��%��
 */
void main_page_redraw_inner_humidity(float humiduty)
{
    lcd_fill(28, 239, 110, 301, TEAL);

    // ������ 3 �ֽ��㹻��ʾ��λ��������ֹ��
    char str[3];

    // ����ʪ���� (0, 99.99] ��Χ�ڲ���ʾ��0 ����Ϊ��Ч/δ��⣿��
    // ע�⣺�����ʵʪ��Ϊ 0%������жϻ����������Ч����Ҫ���� 0%��Ӧ��Ϊ >= 0.0f
    if(humiduty > 0.0f && humiduty <= 99.99f)
        snprintf(str, sizeof(str), "%2.0f", humiduty);

    lcd_show_string(28, 239, str, &font62_maple,1, BLACK, TEAL);

    lcd_show_string(91, 262, "%", &font32_maple,1, BLACK, WHITE);    
}

/**
 * ��������Ϣ����ʾ��������
 * ������const char *city - �����ַ���
 * ˵��/����㣺���ﺯ�����и��ֲ� str[9] �����������ղ�δʹ������ֱ�Ӱ� city ���� lcd_show_string����
 * �����ɶ����ջ���䣻�����Ҫ������ʾ���ȣ�Ӧʹ�� snprintf ��� str ������ str �� lcd_show_string��
 */
void main_page_redraw_outdoor_city(const char *city)
{
    // �ֲ������� 9 �ֽڣ����� '\0'������ζ�������ʾ 8 ���ַ�
    char str[9];
    // ����ʹ�� snprintf �� city ���Ƶ� str �У���ֹ city �����������
    snprintf(str, 9, "%s", city);

    lcd_show_string(127, 170, str, &font20_maple, 1, BLACK, WHITE);
}

/**
 * ���������¶���ʾ����������
 * ������float tempreature - �¶ȣ����϶ȣ�
 */
void main_page_redraw_outdoor_tempreature(float tempreature)
{
    lcd_fill(135, 186, 189, 240, RGB565(253, 135, 75));

    // ��ֵ��������Ĭ����ʾΪ "--"
    char str[3] = {'-','-'};

    // ��Ч��Χ�ж�ͬ�����¶�
    if(tempreature > -10.0f && tempreature <= 100.0f)
        snprintf(str, sizeof(str), "%2.0f", tempreature);

    lcd_show_string(135,186, str, &font54_maple, 1, BLACK, WHITE);

    lcd_show_string(192, 202, "��", &font32_maple, 1, BLACK, WHITE);    
}

/**
 * ������������ code ��������ͼ��
 * ������int code - һ�������������͵Ĵ��루������ API/Ԥ�ȶ��壩
 * ˵����
 *    - ������ʾһ���¶ȼƻ�Ĭ��ͼ�꣨img_icon_wenduji����ĳ��λ�ã�133,239��
 *    - Ȼ����� code ӳ�䵽��ͬ������ͼ�꣨sunny��cloudy��rain �ȣ�
 *    - ��� code ��ƥ���κ���ֵ֪��ʹ�� img_icon_na����ʾ������/δ֪��
 */
void main_page_redraw_outdoor_weather_icon(const int code)
{
    lcd_show_image(133, 239, &img_icon_wenduji);

    const image_t *icon = NULL;

    // ���� code ѡ����ʵ�ͼ��
    if(code == 0 || code == 2 || code == 38)
        icon = &img_icon_sunny;
    else if(code == 1 || code == 3)
        icon = &img_icon_moon;
    else if(code == 4 || code == 9)
        icon = &img_icon_yintian;
    else if(code == 5 || code == 6 || code == 7 || code == 8) 
        icon = &img_icon_cloudy;
    else if(code == 10 || code == 13 || code == 14 || code == 15 || code == 16  || code == 17 || code == 18 || code == 19) 
        icon = &img_icon_rain;
    else if(code == 11 || code == 12) 
        icon = &img_icon_leizhenyu;
    else if(code == 20 || code == 21 || code == 22 || code == 23 || code == 24  || code == 25) 
        icon = &img_icon_snow;
    else
        icon = &img_icon_na;

    lcd_show_image(160, 240, icon);
}
