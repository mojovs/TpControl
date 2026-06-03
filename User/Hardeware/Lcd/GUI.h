#include "Lcd_Driver.h"
#define FSZ 16 //主要字体大小
#define GAP 2 //间距
//画笔颜色

// extern u8 data_ui[32];

void Gui_Circle(u16 X,u16 Y,u16 R,u16 fc);
void Gui_DrawLine(u16 x0, u16 y0,u16 x1, u16 y1,u16 Color);  
void Gui_box(u16 x, u16 y, u16 w, u16 h,u16 bc);
void Gui_box2(u16 x,u16 y,u16 w,u16 h, u8 mode);
void Gui_DisplayButtonDown(u16 x1,u16 y1,u16 x2,u16 y2);
void Gui_DisplayButtonUp(u16 x1,u16 y1,u16 x2,u16 y2);
void Gui_DrawCurve(uint8_t yOffset,short int rawValue);
void Gui_ShowChar(uint16_t x,uint16_t y,uint8_t num,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode);
void Gui_ShowString(uint16_t x,uint16_t y,const uint8_t *p,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode);
/**
 * 直接打印到 屏幕
 * @param x 横坐标
 * @param y 纵坐标
 * @param fmt 输入字符
 * @param ...
 */
void print_string_gui(u8 x,u8 y,const char *fmt,...);
void Gui_ShowChinese(uint16_t x, uint16_t y, u8* s, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode);
void Gui_ShowChinese12x12(uint16_t x,uint16_t y,uint8_t *s,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode);
void Gui_ShowChinese16x16(uint16_t x,uint16_t y,uint8_t *s,uint16_t fc,uint16_t bc,uint8_t sizey,uint8_t mode);
void Gui_showimage_16x16(const unsigned char *p,u8 x,u8 y); //显示16*16
void Gui_showimage_12x12(const unsigned char *path,u8 x,u8 y); //显示12*12
void Gui_showimage_24x24(const unsigned char* path, u8 x, u8 y);

void Gui_DrawFont_Num32(u16 x, u16 y,u8* s, u16 fc, u16 bc );
