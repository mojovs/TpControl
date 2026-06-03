#include "Lcd_Driver.h"
#include "GUI.h"
#include "font.h"
#include "xprintf.h"
#include <stdarg.h>
#include <string.h>

#include "cmsis_os2.h"
#include "delay.h"
#include "MyTask.h"

#include "w25q80_fs.h"
//从ILI93xx读出的数据为GBR格式，而我们写入的时候为RGB格式。
//通过该函数转换
//c:GBR格式的颜色值
//返回值：RGB格式的颜色值
u8 data_ui[32] = {0};


void Gui_Circle(u16 X, u16 Y, u16 R, u16 fc)
{
    //Bresenham算法
    unsigned short a, b;
    int c;
    a = 0;
    b = R;
    c = 3 - 2 * R;
    while (a < b)
    {
        Lcd_DrawPoint(X + a, Y + b, fc); //        7
        Lcd_DrawPoint(X - a, Y + b, fc); //        6
        Lcd_DrawPoint(X + a, Y - b, fc); //        2
        Lcd_DrawPoint(X - a, Y - b, fc); //        3
        Lcd_DrawPoint(X + b, Y + a, fc); //        8
        Lcd_DrawPoint(X - b, Y + a, fc); //        5
        Lcd_DrawPoint(X + b, Y - a, fc); //        1
        Lcd_DrawPoint(X - b, Y - a, fc); //        4

        if (c < 0) c = c + 4 * a + 6;
        else
        {
            c = c + 4 * (a - b) + 10;
            b -= 1;
        }
        a += 1;
    }
    if (a == b)
    {
        Lcd_DrawPoint(X + a, Y + b, fc);
        Lcd_DrawPoint(X + a, Y + b, fc);
        Lcd_DrawPoint(X + a, Y - b, fc);
        Lcd_DrawPoint(X - a, Y - b, fc);
        Lcd_DrawPoint(X + b, Y + a, fc);
        Lcd_DrawPoint(X - b, Y + a, fc);
        Lcd_DrawPoint(X + b, Y - a, fc);
        Lcd_DrawPoint(X - b, Y - a, fc);
    }
}

//画线函数，使用Bresenham 画线算法
void Gui_DrawLine(u16 x0, u16 y0, u16 x1, u16 y1, u16 Color)
{
    int dx, // difference in x's
        dy, // difference in y's
        dx2, // dx,dy * 2
        dy2,
        x_inc, // amount in pixel space to move during drawing
        y_inc, // amount in pixel space to move during drawing
        error, // the discriminant i.e. error i.e. decision variable
        index; // used for looping


    Lcd_SetXY(x0, y0);
    dx = x1 - x0; //计算x距离
    dy = y1 - y0; //计算y距离

    if (dx >= 0)
    {
        x_inc = 1;
    }
    else
    {
        x_inc = -1;
        dx = -dx;
    }

    if (dy >= 0)
    {
        y_inc = 1;
    }
    else
    {
        y_inc = -1;
        dy = -dy;
    }

    dx2 = dx << 1;
    dy2 = dy << 1;

    if (dx > dy) //x距离大于y距离，那么每个x轴上只有一个点，每个y轴上有若干个点
    {
        //且线的点数等于x距离，以x轴递增画点
        // initialize error term
        error = dy2 - dx;

        // draw the line
        for (index = 0; index <= dx; index++) //要画的点数不会超过x距离
        {
            //画点
            Lcd_DrawPoint(x0, y0, Color);

            // test if error has overflowed
            if (error >= 0) //是否需要增加y坐标值
            {
                error -= dx2;

                // move to next line
                y0 += y_inc; //增加y坐标值
            } // end if error overflowed

            // adjust the error term
            error += dy2;

            // move to the next pixel
            x0 += x_inc; //x坐标值每次画点后都递增1
        } // end for
    } // end if |slope| <= 1
    else //y轴大于x轴，则每个y轴上只有一个点，x轴若干个点
    {
        //以y轴为递增画点
        // initialize error term
        error = dx2 - dy;

        // draw the line
        for (index = 0; index <= dy; index++)
        {
            // set the pixel
            Lcd_DrawPoint(x0, y0, Color);

            // test if error overflowed
            if (error >= 0)
            {
                error -= dy2;

                // move to next line
                x0 += x_inc;
            } // end if error overflowed

            // adjust the error term
            error += dx2;

            // move to the next pixel
            y0 += y_inc;
        } // end for
    } // end else |slope| > 1
}


void Gui_box(u16 x, u16 y, u16 w, u16 h, u16 bc)
{
    Gui_DrawLine(x, y, x + w, y, 0xEF7D);
    Gui_DrawLine(x + w - 1, y + 1, x + w - 1, y + 1 + h, 0x2965);
    Gui_DrawLine(x, y + h, x + w, y + h, 0x2965);
    Gui_DrawLine(x, y, x, y + h, 0xEF7D);
    Gui_DrawLine(x + 1, y + 1, x + 1 + w - 2, y + 1 + h - 2, bc);
}

void Gui_box2(u16 x, u16 y, u16 w, u16 h, u8 mode)
{
    if (mode == 0)
    {
        Gui_DrawLine(x, y, x + w, y, 0xEF7D);
        Gui_DrawLine(x + w - 1, y + 1, x + w - 1, y + 1 + h, 0x2965);
        Gui_DrawLine(x, y + h, x + w, y + h, 0x2965);
        Gui_DrawLine(x, y, x, y + h, 0xEF7D);
    }
    if (mode == 1)
    {
        Gui_DrawLine(x, y, x + w, y, 0x2965);
        Gui_DrawLine(x + w - 1, y + 1, x + w - 1, y + 1 + h, 0xEF7D);
        Gui_DrawLine(x, y + h, x + w, y + h, 0xEF7D);
        Gui_DrawLine(x, y, x, y + h, 0x2965);
    }
    if (mode == 2)
    {
        Gui_DrawLine(x, y, x + w, y, 0xffff);
        Gui_DrawLine(x + w - 1, y + 1, x + w - 1, y + 1 + h, 0xffff);
        Gui_DrawLine(x, y + h, x + w, y + h, 0xffff);
        Gui_DrawLine(x, y, x, y + h, 0xffff);
    }
}


/**************************************************************************************
功能描述: 在屏幕显示一凸起的按钮框
输    入: u16 x1,y1,x2,y2 按钮框左上角和右下角坐标
输    出: 无
**************************************************************************************/
void Gui_DisplayButtonDown(u16 x1, u16 y1, u16 x2, u16 y2)
{
    Gui_DrawLine(x1, y1, x2, y1, GRAY2); //H
    Gui_DrawLine(x1 + 1, y1 + 1, x2, y1 + 1, GRAY1); //H
    Gui_DrawLine(x1, y1, x1, y2, GRAY2); //V
    Gui_DrawLine(x1 + 1, y1 + 1, x1 + 1, y2, GRAY1); //V
    Gui_DrawLine(x1, y2, x2, y2, WHITE); //H
    Gui_DrawLine(x2, y1, x2, y2, WHITE); //V
}

/**************************************************************************************
功能描述: 在屏幕显示一凹下的按钮框
输    入: u16 x1,y1,x2,y2 按钮框左上角和右下角坐标
输    出: 无
**************************************************************************************/
void Gui_DisplayButtonUp(u16 x1, u16 y1, u16 x2, u16 y2)
{
    Gui_DrawLine(x1, y1, x2, y1, WHITE); //H
    Gui_DrawLine(x1, y1, x1, y2, WHITE); //V

    Gui_DrawLine(x1 + 1, y2 - 1, x2, y2 - 1, GRAY1); //H
    Gui_DrawLine(x1, y2, x2, y2, GRAY2); //H
    Gui_DrawLine(x2 - 1, y1 + 1, x2 - 1, y2, GRAY1); //V
    Gui_DrawLine(x2, y1, x2, y2, GRAY2); //V
}


static uint16_t lastX = 0, lastY = 0;
static uint8_t firstPoint = 1;

/*
*   函数内容：画折线
*   函数参数：short int rawValue--Y轴参数值
*   返回值：  无
*/
void Gui_DrawCurve(uint8_t yOffset, short int rawValue)
{
    uint16_t x = 0, y = 0;
    y = yOffset - rawValue; //data processing code
    if (firstPoint) //如果是第一次画点，则无需连线，直接描点即可
    {
        Lcd_DrawPoint(0, y,GREEN);
        lastX = 0;
        lastY = y;
        firstPoint = 0;
    }
    else
    {
        x = lastX + 1;

        if (x < 100) //不超过屏幕宽度
        {
            Gui_DrawLine(lastX, lastY, x, y,GREEN);
            lastX = x;
            lastY = y;
        }
        else //超出屏幕宽度，清屏，从第一个点开始绘制，实现动态更新效果
        {
            //TFT_Fill(0,0,160,128,WHITE);//清屏，白色背景
            Lcd_DrawPoint(0, y,GREEN);
            lastX = 0;
            lastY = y;
        }
    }
}

/*
*   函数内容：显示字符
*   函数参数：  x,y---起始坐标
*               num 要显示的字符
*               fc 字的颜色
*               bc 字的背景色
*               sizey 字号
*               mode:  0非叠加模式  1叠加模式
*   返回值：    无
*/
void Gui_ShowChar(uint16_t x, uint16_t y, uint8_t num, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode)
{
    uint8_t sizex, m = 0;
    uint16_t i; //一个字符所占字节大小
    uint16_t x0 = x;
    sizex = sizey / 2;
    u8 width=0;

    //TypefaceNum = (sizex / 8 + ((sizex % 8) ? 1 : 0)) * sizey;
    u8 buf[16] = {0};   //最大部分，其余补0
    font_read_Char((const char *)(&num),buf,sizey); //读取字

    Lcd_SetRegion(x, y, x + sizex - 1, y + sizey - 1); //设置光标位置

    for (i = 0; i < sizey; i++)
    {
        //相素点，横向
        for (u8 j = 0; j < sizex; j++)
        {
            if (!mode) //非叠加方式
            {
                if (buf[i] & (0x01 << j))
                {
                    Lcd_WriteData_16Bit(fc);
                }
               else
                {
                    Lcd_WriteData_16Bit(bc);
                }
                m++;
                if (m % sizey == 0)
                {
                    m = 0;
                    break;
                }
            }
            else //叠加方式
            {
                if (buf[i] & (0x01 << j))
                {
                    Lcd_DrawPoint(x, y, fc); //画一个点
                }
                x++;
                if ((x - x0) == sizey)
                {
                    x = x0;
                    y++;
                    break;
                }
            }
        }
    }

}

/*
*   函数内容：  显示字符串
*   函数参数：  x,y---起始坐标
*               *p 要显示的字符串
*               fc 字的颜色
*               bc 字的背景色
*               sizey 字号
*               mode:  0非叠加模式  1叠加模式
*   返回值：    无
*/
void Gui_ShowString(uint16_t x, uint16_t y, const uint8_t* p, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode)
{
    while (*p != '\0')
    {
        Gui_ShowChar(x, y, *p, fc, bc, sizey, mode);
        x += sizey / 2;
        p++;
    }
}

/******************************************************************************
      函数说明：显示汉字串
      入口数据：x,y显示坐标
                *s 要显示的汉字串
                fc 字的颜色
                bc 字的背景色
                sizey 字号 可选 16 24 32
                mode:  0非叠加模式  1叠加模式
      返回值：  无
******************************************************************************/
void Gui_ShowChinese(uint16_t x, uint16_t y, u8* s, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode)
{

    while (*s != 0)
    {
        if (sizey == 12)
        {
            Gui_ShowChinese12x12(x, y, s, fc, bc, sizey, mode);
        }
        else if (sizey == 16)
        {
            Gui_ShowChinese16x16(x, y, s, fc, bc, sizey, mode);
        }

        s += 3;
        x += sizey;
    }
}

/******************************************************************************
      函数说明：显示单个12x12汉字
      入口数据：x,y显示坐标
                *s 要显示的汉字
                fc 字的颜色
                bc 字的背景色
                sizey 字号
                mode:  0非叠加模式  1叠加模式
      返回值：  无
******************************************************************************/
void Gui_ShowChinese12x12(uint16_t x, uint16_t y, uint8_t* s, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode)
{
    uint8_t i = 0, j = 0, m = 0;
    uint16_t TypefaceNum = 0; //一个字符所占字节大小
    uint16_t x0 = x;
    TypefaceNum = (sizey / 8 + ((sizey % 8) ? 1 : 0)) * sizey; //24 , 1

    u8 buf[24] = {0};
    font_read_CN(s, buf,12); //将字模读取到buf

    Lcd_SetRegion(x, y, x + sizey - 1, y + sizey - 1); //设定区域


    for (i = 0; i < TypefaceNum; i++)
    {
        //相素点，从上到下，从左到右,每列像素，不足8位的补为8位，比如12*12,那么就会直接补为16
        for (j = 0; j < 8; j++)
        {
            if (!mode) //非叠加方式
            {
                if (buf[i] & (0x1 <<j))
                {
                    Lcd_WriteData_16Bit(fc);
                }
                else
                {
                    Lcd_WriteData_16Bit(bc);
                }
                m++;
                if (m % sizey == 0)
                {
                    m = 0;
                    break;
                }
            }
            else //叠加方式
            {
                if (buf[i] & (0x01 << j))
                {
                    Lcd_DrawPoint(x, y, fc); //画一个点
                }
                x++;
                if ((x - x0) == sizey)
                {
                    x = x0;
                    y++;
                    break;
                }
            }
        }
    }
}


/******************************************************************************
      函数说明：显示单个16x16汉字
      入口数据：x,y显示坐标
                *s 要显示的汉字
                fc 字的颜色
                bc 字的背景色
                sizey 字号
                mode:  0非叠加模式  1叠加模式
      返回值：  无
******************************************************************************/
void Gui_ShowChinese16x16(uint16_t x, uint16_t y, uint8_t* s, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode)
{
    uint8_t i = 0, j = 0, m = 0;
    uint16_t k = 0;
    uint16_t HZnum = 0; //汉字数目
    uint16_t TypefaceNum = 0; //一个字符所占字节大小
    uint16_t x0 = x;
    TypefaceNum = (sizey / 8 + ((sizey % 8) ? 1 : 0)) * sizey; //32

    u8 buf[32] = {0};
    font_read_CN(s, buf,16); //将字模读取到buf
    {
        Lcd_SetRegion(x, y, x + sizey - 1, y + sizey - 1);
        for (i = 0; i < TypefaceNum; i++)
        {
            for (j = 0; j < 8; j++)
            {
                if (!mode) //非叠加方式
                {
                    if (buf[i] & (0x01 << j))
                    {
                        Lcd_WriteData_16Bit(fc);
                    }
                    else
                    {
                        Lcd_WriteData_16Bit(bc);
                    }
                    m++;
                    if (m % sizey == 0)
                    {
                        m = 0;
                        break;
                    }
                }
                else //叠加方式
                {
                    if (buf[i] & (0x01 << j))
                    {
                        Lcd_DrawPoint(x, y, fc); //画一个点
                    }
                    x++;
                    if ((x - x0) == sizey)
                    {
                        x = x0;
                        y++;
                        break;
                    }
                }
            }
        }
    }
}

//取模方式 水平扫描 从左到右 低位在前
void Gui_showimage_16x16(const unsigned char* p, u8 x, u8 y) //显示16*16 QQ图片
{
    //Lcd_Fill(x,y,x+16,y+16,COLOR_MAIN);
    Lcd_SetRegion(x, y, x + 15, y + 15);
    u8 row = 0, col = 0;
    for (row = 0; row < 16; row++) //ROW loop
    {
        for (col = 0; col < 16; col++) //column loop
        {
            Lcd_WriteData(*p++);
            Lcd_WriteData(*p++);
        }
    }
}
void Gui_Showimage(const char *path,u8 x,u8 y)
{



}

void Gui_showimage_12x12(const unsigned char* path, u8 x, u8 y)
{
    //打开文件
    lfs_file_t file;
    u8 buf[288]={0};


    int16_t err= lfs_file_open(&lfs,&file,path,LFS_O_RDONLY);
    if (err)
    {
        ble_send("file open err\n");
    }
    err= lfs_file_read(&lfs,&file,buf,288);
    if (err)
    {
        //ble_send("file read %d\n",err);
    }
    lfs_file_close(&lfs,&file);
    //Lcd_Fill(x,y,x+16,y+16,COLOR_MAIN);
    Lcd_SetRegion(x, y, x + 11, y + 11);
    u8 row = 0, col = 0;
    for (row = 0; row < 12; row++) //ROW loop
    {
        for (col = 0; col < 12; col++) //column loop
        {
            //高字节在后
            //目前为LSB,如果取模为MSB，那么 将下方两行代码调换位置
            Lcd_WriteData(buf[2*col+2*row*12+1]);
            Lcd_WriteData(buf[2*col+2*row*12]);
        }
    }
}
void Gui_showimage_24x24(const unsigned char* path, u8 x, u8 y)
{
    //打开文件
    lfs_file_t file;
    u8 buf[1152]={0};


    int16_t err= lfs_file_open(&lfs,&file,path,LFS_O_RDONLY);
    if (err)
    {
        ble_send("file open err\n");
    }
    err= lfs_file_read(&lfs,&file,buf,1152);
    if (err)
    {
        ///ble_send("file read %d\n",err);
    }
    lfs_file_close(&lfs,&file);
    //Lcd_Fill(x,y,x+16,y+16,COLOR_MAIN);
    Lcd_SetRegion(x, y, x + 23, y + 23);
    u8 row = 0, col = 0;
    for (row = 0; row < 24; row++) //ROW loop
    {
        for (col = 0; col < 24; col++) //column loop
        {
            //高字节在后
            //目前为LSB,如果取模为MSB，那么 将下方两行代码调换位置
            Lcd_WriteData(buf[2*col+2*row*24+1]);
            Lcd_WriteData(buf[2*col+2*row*24]);
        }
    }
}

void print_string_gui(u8 x, u8 y, const char* fmt, ...)
{

     //osMutexAcquire(mutex_tft, osWaitForever);
    va_list arp;
    memset(data_ui, 0, sizeof(data_ui)); //清理data_UI
    va_start(arp, fmt);
    xsprintf_m(data_ui, fmt, arp); //打印到gui数据
    va_end(arp);
    Gui_ShowString(x, y, data_ui,RED,WHITE, 12, 0);
     //osMutexRelease(mutex_tft);
}

void Gui_DrawFont_Num32(u16 x, u16 y,u8* s, u16 fc, u16 bc )
{
    unsigned char i,j,k,c;
    //lcd_text_any(x+94+i*42,y+34,32,32,0x7E8,0x0,sz32,knum[i]);
    //	w=w/8;
    u8 buf[128]={0};
    font_read_DIG_Hum(s,buf);

    //32行
    for(int i=0;i<32;i++)           // 64行
    {
        for(int j=0;j<4;j++)        // 4字节表示一行
        {
            uint8_t c = buf[i*4 + j];
            for(int k=0;k<8;k++)
            {
                if(c & (0x80 >> k))
                    Lcd_DrawPoint(x+j*8+k, y+i, fc);
                else if(fc != bc)
                    Lcd_DrawPoint(x+j*8+k, y+i, bc);
            }
        }
    }
}


