#include "lcd.h"
#include "delay.h"

lcd_dev_t lcddev;

uint16_t POINT_COLOR = 0x0000; // 画笔颜色
uint16_t BACK_COLOR = 0xFFFF;  // 背景颜色

//基本寄存器读写
// 写寄存器编号（告诉 LCD 下一步要对哪个寄存器操作）
static void lcd_write_regaddress(vu16 regaddress)
{
    regaddress = regaddress;            // 防止 -O2 优化时编译器优化掉
    LCD->LCD_REG = regaddress;      // 写入寄存器地址
}

// 写数据到 LCD 寄存器
static void lcd_write_data(vu16 data)
{
    data = data;
    LCD->LCD_RAM = data;        // 写入寄存器数据
}

// 读数据
static u16 lcd_read_data(void)
{
    vu16 ram;                   // volatile 避免被优化
    ram = LCD->LCD_RAM;         //读出数据
    return ram;
}

// 写寄存器（地址+数据）
void lcd_write_register(vu16 regaddress, vu16 data)
{
    LCD->LCD_REG = regaddress;      // 写寄存器地址
    LCD->LCD_RAM = data;        // 写入数据
}

// 读寄存器
u16 lcd_read_register(vu16 regaddress)
{
    lcd_write_regaddress(regaddress);     // 写入寄存器地址
    delay_us(5);
    return lcd_read_data();             // 读寄存器值
}

//GRAM 相关
// 进入写GRAM模式（光标所指的像素位置）
void lcd_write_ram_prepare(void)
{
    LCD->LCD_REG = lcddev.wramcmd;
}

// 写颜色值到GRAM（画点）
void lcd_write_ram(u16 RGB_code)
{
    LCD->LCD_RAM = RGB_code;          // 写16位颜色
}

// RGB565 转换函数
u16 RGB565(u8 r, u8 g, u8 b)
{
    // r:0-255 g:0-255 b:0-255
    // r & 0xF8: 取高 5 位
    // g & 0xFC: 取高 6 位
    // b >> 3: 取高 5 位（低位舍弃）
    return ((r & 0xF8) << 8) |   // R 5位
           ((g & 0xFC) << 3) |   // G 6位
           (b >> 3);             // B 5位
}

//开关显示
// 开启显示
void lcd_display_on(void)
{
    lcd_write_regaddress(0x29); // Display ON
}

// 关闭显示
void lcd_display_off(void)
{
    lcd_write_regaddress(0x28); // Display OFF
}

//设置光标位置
// 设置光标到 (x_pos, y_pos)
void lcd_set_cursor(u16 x_pos, u16 y_pos)
{
    // 写列地址（高 8 位、低 8 位）
    lcd_write_regaddress(lcddev.setxcmd);
    lcd_write_data(x_pos >> 8); 
    lcd_write_data(x_pos & 0xFF);

    // 写行地址（高 8 位、低 8 位）
    lcd_write_regaddress(lcddev.setycmd);
    lcd_write_data(y_pos >> 8); 
    lcd_write_data(y_pos & 0xFF); 
}

void lcd_scan_dir(u8 direction)
{
    u16 regval = 0;
    u16 dirreg = 0;
    switch(direction)
    {
        case L2R_U2D://从左到右,从上到下
            regval |= (0 << 7) | (0 << 6) | (0 << 5); 
            break;
        case L2R_D2U://从左到右,从下到上
            regval |= (1 << 7) | (0 << 6) | (0 << 5); 
            break;
        case R2L_U2D://从右到左,从上到下
            regval |= (0 << 7) | (1 << 6) | (0 << 5); 
            break;
        case R2L_D2U://从右到左,从下到上
            regval |= (1 << 7) | (1 << 6) | (0 << 5); 
            break;     
        case U2D_L2R://从上到下,从左到右
            regval |= (0 << 7) | (0 << 6) | (1 << 5); 
            break;
        case U2D_R2L://从上到下,从右到左
            regval |= (0 << 7) | (1 << 6) | (1 << 5); 
            break;
        case D2U_L2R://从下到上,从左到右
            regval |= (1 << 7) | (0 << 6) | (1 << 5); 
            break;
        case D2U_R2L://从下到上,从右到左
            regval |= (1 << 7) | (1 << 6) | (1 << 5); 
            break;     
    }

	regval |= 0x08;  // BGR顺序
    dirreg = 0X36;       
    lcd_write_register(dirreg,regval);
} 

//画点
//POINT_COLOR:此点的颜色
void lcd_draw_point(u16 x,u16 y,u16 color)
{
    lcd_set_cursor(x,y);        //设置光标位置 
    lcd_write_ram_prepare();    //开始写入GRAM
    LCD->LCD_RAM = color; 
}

//快速画点
//color:颜色
void lcd_fast_draw_point(u16 x,u16 y,u16 color)
{       
    lcd_write_regaddress(lcddev.setxcmd);
    lcd_write_data(x >> 8);  
    lcd_write_data(x & 0XFF);     
    lcd_write_regaddress(lcddev.setycmd); 
    lcd_write_data(y >> 8); 
    lcd_write_data(y & 0XFF);
            
    LCD->LCD_REG = lcddev.wramcmd; 
    LCD->LCD_RAM = color; 
}

//设置LCD显示方向
//dir:0,竖屏；1,横屏
void lcd_display_dir(u8 direction)
{
    if(direction == 0)            //竖屏
    {
        lcddev.direction = 0;    //竖屏
        lcddev.width = 240;
        lcddev.height = 320;
    }
	else                 //横屏
    {                      
        lcddev.direction = 1;    //横屏
        lcddev.width = 320;
        lcddev.height = 240;
    } 
    lcddev.wramcmd = 0x2C;
    lcddev.setxcmd = 0x2A;
    lcddev.setycmd = 0x2B; 

    lcd_scan_dir(DFT_SCAN_DIR);    //默认扫描方向,从左到右，从上到下
}

void lcd_init(void)
{
    vu32 i=0;
    
	GPIO_InitTypeDef  GPIO_InitStructure;
    FSMC_NORSRAMInitTypeDef  FSMC_NORSRAMInitStructure;
	FSMC_NORSRAMTimingInitTypeDef  readWriteTiming; 
    FSMC_NORSRAMTimingInitTypeDef  writeTiming;
    
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB|RCC_AHB1Periph_GPIOD|RCC_AHB1Periph_GPIOE, ENABLE);//使能IO时钟  
	RCC_AHB3PeriphClockCmd(RCC_AHB3Periph_FSMC,ENABLE);//使能FSMC时钟  
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1; //PB1 推挽输出,控制背光
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT; //普通输出模式
 	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽输出
 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; //100MHz
 	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //上拉
 	GPIO_Init(GPIOB, &GPIO_InitStructure); //初始化 
    
    // 配置 GPIOD 多个引脚为 FSMC 复用功能（数据/地址/控制线）
    // PD0, PD1, PD4, PD5, PD7, PD8, PD9, PD10, PD13, PD14, PD15 等
 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_7 | GPIO_Pin_8
                                | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15; 
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF; //复用输出
 	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽输出
 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz; //100MHz
 	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //上拉
 	GPIO_Init(GPIOD, &GPIO_InitStructure); //初始化  
    
     // 配置 GPIOE 的 PE7~PE15 为 FSMC 复用 
 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12
                                | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15; //PE7~15,AF OUT
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF; //复用输出
 	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽输出
 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz; //100MHz
 	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //上拉
 	GPIO_Init(GPIOE, &GPIO_InitStructure); //初始化  

    // 将上面配置的引脚映射到 FSMC 外设 AF（AF12）
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource0,GPIO_AF_FSMC);
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource1,GPIO_AF_FSMC);
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource4,GPIO_AF_FSMC);
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource5,GPIO_AF_FSMC);
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource7,GPIO_AF_FSMC);  
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource8,GPIO_AF_FSMC); 
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource9,GPIO_AF_FSMC);
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource10,GPIO_AF_FSMC);
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource13,GPIO_AF_FSMC);   
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource14,GPIO_AF_FSMC);
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource15,GPIO_AF_FSMC);
 
	GPIO_PinAFConfig(GPIOE,GPIO_PinSource7,GPIO_AF_FSMC);
	GPIO_PinAFConfig(GPIOE,GPIO_PinSource8,GPIO_AF_FSMC);
	GPIO_PinAFConfig(GPIOE,GPIO_PinSource9,GPIO_AF_FSMC);
	GPIO_PinAFConfig(GPIOE,GPIO_PinSource10,GPIO_AF_FSMC);
	GPIO_PinAFConfig(GPIOE,GPIO_PinSource11,GPIO_AF_FSMC);
	GPIO_PinAFConfig(GPIOE,GPIO_PinSource12,GPIO_AF_FSMC);
	GPIO_PinAFConfig(GPIOE,GPIO_PinSource13,GPIO_AF_FSMC);
	GPIO_PinAFConfig(GPIOE,GPIO_PinSource14,GPIO_AF_FSMC);
	GPIO_PinAFConfig(GPIOE,GPIO_PinSource15,GPIO_AF_FSMC);

    // 配置 FSMC 的时序
    // 读/写通用时序（主要用于读操作）：
	readWriteTiming.FSMC_AddressSetupTime = 0xF; //地址设置时间,地址信号在数据传输开始前保持稳定的最小时间    16个HCLK 1/168M * 16 = 6ns * 16 = 96ns    
	readWriteTiming.FSMC_AddressHoldTime = 0x00; // 地址保持时间,地址信号在数据传输结束后保持稳定的最小时间    模式A常设为0   
	readWriteTiming.FSMC_DataSetupTime = 60; //数据设置时间,数据信号在数据传输开始前保持稳定的最小时间    60个HCLK = 6 * 60 = 360ns
	readWriteTiming.FSMC_BusTurnAroundDuration = 0x00; //总线转向时间,多设备共享总线情况下设置以避免冲突
	readWriteTiming.FSMC_CLKDivision = 0x00; //时钟分频因子 不分频
	readWriteTiming.FSMC_DataLatency = 0x00; //数据延迟时间,数据信号在地址信号后的延迟时间
	readWriteTiming.FSMC_AccessMode = FSMC_AccessMode_A; //访问模式:模式A
    

	writeTiming.FSMC_AddressSetupTime = 2; //地址设置时间   2个HCLK = 12ns 
	writeTiming.FSMC_AddressHoldTime = 0x00; //地址保持时间      
	writeTiming.FSMC_DataSetupTime = 3; //数据设置时间     6ns * 3个HCLK = 18ns
	writeTiming.FSMC_BusTurnAroundDuration = 0x00;
	writeTiming.FSMC_CLKDivision = 0x00;
	writeTiming.FSMC_DataLatency = 0x00;
	writeTiming.FSMC_AccessMode = FSMC_AccessMode_A; 

 
	FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM1; //使用 Bank1 的 NE1（对应片选 NE1） 
	FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable; //地址/数据线不复用（独立）
	FSMC_NORSRAMInitStructure.FSMC_MemoryType =FSMC_MemoryType_SRAM; //存储器类型：SRAM（并口型 LCD 常用） 
	FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_16b;//存储器数据宽度为16bit   
	FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode =FSMC_BurstAccessMode_Disable; //突发访问模式 不允许
	FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low; //等待信号的极性,仅在突发模式访问下有用
	FSMC_NORSRAMInitStructure.FSMC_AsynchronousWait=FSMC_AsynchronousWait_Disable; //同步传输等待信号，未用到
	FSMC_NORSRAMInitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;  //环绕模式 未用到
	FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState; //等待信号激活时间:等待信号在等待状态前激活
	FSMC_NORSRAMInitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable; //存储器写使能
	FSMC_NORSRAMInitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable; //等待信号 不启用
	FSMC_NORSRAMInitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Enable; // 读写使用不同的时序(拓展模式)
	FSMC_NORSRAMInitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable; 
	FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &readWriteTiming; //读时序
	FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &writeTiming;  //写时序

	FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure);  //初始化FSMC配置

	FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM1, ENABLE);  //使能 Bank1，FSMC 生效
    
    delay_ms(50);
    lcd_write_register(0x0000,0x0001);
    delay_ms(50);
  
    // 直接读 ID，只认 0x9341
    lcddev.id = 0;
    lcd_write_regaddress(0xD3);          // 读取 ILI9341 ID
    (void)lcd_read_data();       // dummy read
    (void)lcd_read_data();      // 0x00
    lcddev.id  = lcd_read_data(); // 0x93
    lcddev.id <<= 8;
    lcddev.id |= lcd_read_data(); // 0x41

    if(lcddev.id == 0x9341)
    {
        // 配置 FSMC 写时序（最快）
        FSMC_Bank1E->BWTR[6] &= ~(0xF << 0); // 清除地址建立时间
        FSMC_Bank1E->BWTR[6] &= ~(0xF << 8); // 清除数据保存时间
        FSMC_Bank1E->BWTR[6] |= 3 << 0;      // 地址建立时间：3 HCLK = 18ns
        FSMC_Bank1E->BWTR[6] |= 2 << 8;      // 数据保存时间：3 HCLK = 18ns
    }
    else
    {
        // 如果不是 9341，可以在这里加报错处理
        lcddev.id = 0xFFFF;
    }
    //printf(" LCD ID:%x\r\n",lcddev.id); //打印LCD ID  
    if(lcddev.id==0X9341)    //9341初始化
    {     
        lcd_write_regaddress(0xCF);  
        lcd_write_data(0x00);
        i++;
        lcd_write_data(0xC1); 
        lcd_write_data(0X30); 
        lcd_write_regaddress(0xED);  
        lcd_write_data(0x64); 
        lcd_write_data(0x03); 
        lcd_write_data(0X12); 
        lcd_write_data(0X81); 
        lcd_write_regaddress(0xE8);  
        lcd_write_data(0x85); 
        lcd_write_data(0x10); 
        lcd_write_data(0x7A); 
        lcd_write_regaddress(0xCB);  
        lcd_write_data(0x39); 
        lcd_write_data(0x2C); 
        lcd_write_data(0x00); 
        lcd_write_data(0x34); 
        lcd_write_data(0x02); 
        lcd_write_regaddress(0xF7);  
        lcd_write_data(0x20); 
        lcd_write_regaddress(0xEA);  
        lcd_write_data(0x00); 
        lcd_write_data(0x00); 
        lcd_write_regaddress(0xC0);    //Power control 
        lcd_write_data(0x1B);   //VRH[5:0] 
        lcd_write_regaddress(0xC1);    //Power control 
        lcd_write_data(0x01);   //SAP[2:0];BT[3:0] 
        lcd_write_regaddress(0xC5);    //VCM control 
        lcd_write_data(0x30);      //3F
        lcd_write_data(0x30);      //3C
        lcd_write_regaddress(0xC7);    //VCM control2 
        lcd_write_data(0XB7); 
        lcd_write_regaddress(0x36);    // Memory Access Control 
        lcd_write_data(0x48); 
        lcd_write_regaddress(0x3A);   
        lcd_write_data(0x55); 
        lcd_write_regaddress(0xB1);   
        lcd_write_data(0x00);   
        lcd_write_data(0x1A); 
        lcd_write_regaddress(0xB6);    // Display Function Control 
        lcd_write_data(0x0A); 
        lcd_write_data(0xA2); 
        lcd_write_regaddress(0xF2);    // 3Gamma Function Disable 
        lcd_write_data(0x00); 
        lcd_write_regaddress(0x26);    //Gamma curve selected 
        lcd_write_data(0x01); 
        lcd_write_regaddress(0xE0);    //Set Gamma 
        lcd_write_data(0x0F); 
        lcd_write_data(0x2A); 
        lcd_write_data(0x28); 
        lcd_write_data(0x08); 
        lcd_write_data(0x0E); 
        lcd_write_data(0x08); 
        lcd_write_data(0x54); 
        lcd_write_data(0XA9); 
        lcd_write_data(0x43); 
        lcd_write_data(0x0A); 
        lcd_write_data(0x0F); 
        lcd_write_data(0x00); 
        lcd_write_data(0x00); 
        lcd_write_data(0x00); 
        lcd_write_data(0x00);          
        lcd_write_regaddress(0XE1);    //Set Gamma 
        lcd_write_data(0x00); 
        lcd_write_data(0x15); 
        lcd_write_data(0x17); 
        lcd_write_data(0x07); 
        lcd_write_data(0x11); 
        lcd_write_data(0x06); 
        lcd_write_data(0x2B); 
        lcd_write_data(0x56); 
        lcd_write_data(0x3C); 
        lcd_write_data(0x05); 
        lcd_write_data(0x10); 
        lcd_write_data(0x0F); 
        lcd_write_data(0x3F); 
        lcd_write_data(0x3F); 
        lcd_write_data(0x0F); 
        lcd_write_regaddress(0x2B); 
        lcd_write_data(0x00);
        lcd_write_data(0x00);
        lcd_write_data(0x01);
        lcd_write_data(0x3f);
        lcd_write_regaddress(0x2A); 
        lcd_write_data(0x00);
        lcd_write_data(0x00);
        lcd_write_data(0x00);
        lcd_write_data(0xef);     
        lcd_write_regaddress(0x11); //Exit Sleep
        delay_ms(120);
        lcd_write_regaddress(0x29); //display on    
    }       
    lcd_display_dir(0); //默认为竖屏

    GPIO_SetBits(GPIOB, GPIO_Pin_1); //点亮背光
    lcd_clear(WHITE);
}

//清屏函数
//color:要清屏的填充色
void lcd_clear(u16 color)
{
    u32 index = 0;      
    u32 totalpoint = lcddev.width;
    totalpoint *= lcddev.height;             // 得到总像素数 = 宽 * 高
    lcd_set_cursor(0x00,0x0000);             // 将光标移动到 (0,0)
    lcd_write_ram_prepare();                  // 进入 WRAM 写模式   
    for(index = 0; index < totalpoint; index++)
    {
        LCD->LCD_RAM = color;    // 连续写入颜色（利用 FSMC 自动推进地址以填充整屏）
    }
}

void lcd_fill(u16 sx, u16 sy, u16 width, u16 height, u16 color)
{          
    u16 i, j;

    for(i = sy; i < height; i++)                 // 共写 height 行
    {
        lcd_set_cursor(sx, i);              // 设置光标位置（逐行移动）
        lcd_write_ram_prepare();                // 开始写入 GRAM      
        for(j = 0; j < width; j++)               // 写入一行颜色
        {
            LCD->LCD_RAM = color;               // 显示颜色         
        }
    }   
}

static int get_font_index(u8 ch, const font_t *font)
{
    if (font == NULL) return -1;

    // 如果字库提供了自定义映射表（ascii_map），则按映射表查找
    if (font->ascii_map != NULL)
    {
        for (u8 i = 0; i < font->ascii_count; i++)
        {
            if (font->ascii_map[i] == ch)
                return i;    // 返回映射索引
        }
        return -1;    // 字符不在映射表中
    }

    // 默认从空格开始
    int index = ch - ' ';
    if (index < 0 || index >= font->ascii_count)
        return -1;
    return index;
}


void lcd_show_char(u16 x, u16 y, u8 ch, const font_t *font, u8 mode, u16 color, u16 bgcolor)
{
    if (font == NULL || font->ascii_model == NULL)
        return;

    int index = get_font_index(ch, font);
    if (index < 0)
        return; // 字体不支持该字符

    u16 size = font->size;
    u16 y0 = y;
    // 计算每个字符占用多少字节：每列占 (size/8 (+1 if rem)) 字节，列数 = size/2（假设等宽字体）
    u16 bytes_per_char = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2);

    const u8 *char_data = font->ascii_model + index * bytes_per_char;    // 字符起始字节地址

    for (u16 t = 0; t < bytes_per_char; t++)
    {
        u8 temp = char_data[t];

        for (u8 bit = 0; bit < 8; bit++)
        {
            if (temp & 0x80)
                lcd_fast_draw_point(x, y, color);
            else if (mode == 0)
                lcd_fast_draw_point(x, y, bgcolor);     // mode==0 表示透明背景需要填充背景色

            temp <<= 1;         // 移动到下一个位
            y++;                // 垂直移动一个像素
            if ((y - y0) == size)
            {
                // 一列已经绘制完（绘制了 size 个像素），恢复到起始 y 并移动到下一列 x++
                y = y0;
                x++;
                break;    // 处理下一字节（下一列）
            }
        }
    }
}

void lcd_show_chinese_char(u16 x, u16 y, const char *name, const font_t *font, u8 mode, u16 color, u16 bgcolor)
{
    if (font == NULL || font->chinese == NULL || name == NULL) return;

    const font_chinese_t *ch = font->chinese;    // 指向汉字字库数组
    u16 x0 = x, y0 = y;
    u16 size = font->size;
    u16 bytes_per_row = (size + 7) / 8;  // 每行占用字节数 （向上取整）
    u16 bytes_per_char = bytes_per_row * size; // 每个汉字占用的总字节数

    // 遍历字库数组，查找匹配的名字（通常 name 为两个字节）
    for (; ch->name != NULL && ch->model != NULL; ch++)
    {
        if (ch->name[0] == name[0] && ch->name[1] == name[1])
        {
            // 找到匹配字模
            const uint8_t *data = ch->model;
            for (u16 row = 0; row < size; row++) // 共 size 行
            {
                for (u16 col_byte = 0; col_byte < bytes_per_row; col_byte++) // 每行有 bytes_per_row 个字节
                {
                    u8 temp = data[row * bytes_per_row + col_byte];
                    // 高位在前
                    for (u8 bit = 0; bit < 8; bit++)
                    {
                        if (col_byte * 8 + bit >= size) break; // 防止该行最后一个字节的多余位被处理
                        if (temp & (0x80 >> bit))
                            lcd_fast_draw_point(x, y, color);
                        else if (mode == 0)
                            lcd_fast_draw_point(x, y, bgcolor);

                        x++;
                        if (x >= lcddev.width) return;    // 越界保护
                    }
                }
                y++;       //下一行
                x = x0;    // 恢复到该汉字左侧起始列
                if (y >= lcddev.height) return;
            }
            return; // 显示完成
        }
    }
}

void lcd_show_string(u16 x, u16 y, const u8 *String, const font_t *font, u8 mode, u16 color, u16 bgcolor)
{
    if (font == NULL || String == NULL) return;

    u16 x_start = x;  // 每行起始 X 坐标，换行时使用
    u16 size = font->size;  // 字体高度

    for (u16 i = 0; String[i] != '\0'; )
    {
        // GB2312 编码判断：如果最高位为 1，则视为中文的第一个字节（两字节）
        u8 char_len = (String[i] & 0x80) ? 2 : 1; 
        if (char_len == 2 && String[i+1] == '\0') break; // 防止越界

        // 计算当前字符宽度：单字节使用半宽（size/2），双字节中文使用全宽（size）
        u16 char_width = (char_len == 1) ? (size / 2) : size;

        // 换行判断：如果当前 x + 字宽 超过屏幕宽度，则换行
        if (x + char_width > lcddev.width)
        {
            x = x_start;  // 回到左侧起始位置
            y += size;    // 向下一个文本行移动一个字体高度
        }

        // 超出屏幕高度则停止显示
        if (y + size > lcddev.height)
            break;

        // 显示字符
        if (char_len == 1)
        {
            // 英文或符号
            lcd_show_char(x, y, String[i], font, mode, color, bgcolor);
        }
        else
        {
            // 中文（传入两个字节）
            lcd_show_chinese_char(x, y, (const char *)&String[i], font, mode, color, bgcolor);
        }

        // 更新坐标和索引
        x += char_width;
        i += char_len;
    }
}

// 假设图片宽高为 LCD 宽高
void lcd_show_image(u16 x, u16 y, const image_t *img)
{
    if (img == NULL || img->data == NULL)
        return;

    const uint8_t *ptr = img->data;      // 指向图片数据缓冲区
    uint16_t color;

    for (uint16_t j = 0; j < img->height; j++)
    {
        for (uint16_t i = 0; i < img->width; i++)
        {
            // RGB565: 每个像素占 2 字节（低位在前）
            color = (ptr[1] << 8) | ptr[0];
            lcd_fast_draw_point(x + i, y + j, color);
            ptr += 2;   // 跳到下一个像素
        }
    }
}