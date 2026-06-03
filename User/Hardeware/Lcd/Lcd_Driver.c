//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//  功能描述   : 1.8寸LCD 4接口演示例程(STM32系列)
/******************************************************************************
//本程序适用与STM32F103C8
//              GND   电源地
//              VCC   接5V或3.3v电源
//              SCL   接PA5（SCL）
//              SDA   接PA7（SDA）
//              RES   接PB0
//              DC    接PB1
//              CS    接PA4 
//							BL		接PB10
*******************************************************************************/
#include "Lcd_Driver.h"
#include "spi.h"
#include "cmsis_os2.h"
#include "delay.h"
#include "MyTask.h"
#include "spi_sel.h"

//液晶IO初始化配置
void LCD_GPIO_Init(void)
{
	// __HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	//初始化gpio
	GPIO_InitTypeDef gpio_initer;
	gpio_initer.Pin=GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
	gpio_initer.Mode=GPIO_MODE_OUTPUT_PP;
	//gpio_initer.Pull=GPIO_PULLUP;
	gpio_initer.Speed=GPIO_SPEED_HIGH;
	HAL_GPIO_Init(GPIOB,&gpio_initer);


}
//向SPI总线传输一个8位数据
void  SPI_WriteData(u8 data)
{
	HAL_SPI_Transmit(&spi_handler_1,&data,1,10);
}

//向液晶屏写一个8位指令
void Lcd_WriteIndex(u8 Index)
{
   //SPI 写命令时序开始
	SPI1_Sel_Device(SPI1_TFT);
	LCD_RS_CLR;
	 SPI_WriteData(Index);
	SPI1_Unsel_Device(SPI1_TFT);
}

void Lcd_WriteIndex_Lock(u8 Index)
{
	osMutexAcquire(mutex_spi1,osWaitForever);
	//SPI 写命令时序开始
	SPI1_Sel_Device(SPI1_TFT);
	LCD_RS_CLR;
	SPI_WriteData(Index);
	SPI1_Unsel_Device(SPI1_TFT);
	osMutexRelease(mutex_spi1);
}

//向液晶屏写一个8位数据
void Lcd_WriteData(u8 Data)
{
	SPI1_Sel_Device(SPI1_TFT);
   LCD_RS_SET;
   SPI_WriteData(Data);
	SPI1_Unsel_Device(SPI1_TFT);
}

void Lcd_WriteData_Lock(u8 Data)
{
	osMutexAcquire(mutex_spi1,osWaitForever);
	SPI1_Sel_Device(SPI1_TFT);
	LCD_RS_SET;
	SPI_WriteData(Data);
	SPI1_Unsel_Device(SPI1_TFT);
	osMutexRelease(mutex_spi1);
}

//向液晶屏写一个16位数据
void Lcd_WriteData_16Bit(u16 Data)
{
	SPI1_Sel_Device(SPI1_TFT);
   LCD_RS_SET;
	 SPI_WriteData(Data>>8); 	//写入高8位数据
	 SPI_WriteData(Data); 			//写入低8位数据
	SPI1_Unsel_Device(SPI1_TFT);
}

void Lcd_WriteData_16Bit_Lock(u16 Data)
{
	osMutexAcquire(mutex_spi1,osWaitForever);
	SPI1_Sel_Device(SPI1_TFT);
	LCD_RS_SET;
	SPI_WriteData(Data>>8); 	//写入高8位数据
	SPI_WriteData(Data); 			//写入低8位数据
	SPI1_Unsel_Device(SPI1_TFT);
	osMutexRelease(mutex_spi1);
}

void Lcd_WriteReg(u8 Index,u8 Data)
{
	osMutexAcquire(mutex_spi1,osWaitForever);
	Lcd_WriteIndex(Index);
	Lcd_WriteData(Data);
	osMutexRelease(mutex_spi1);
}

void Lcd_Reset(void)
{
	LCD_SCL_CLR;
	LCD_RST_CLR;
    delay_ms(200);
	LCD_RST_SET;
	delay_ms(100);
}

void Lcd_SleepOut(void)
{
	Lcd_WriteIndex(0x11);//Sleep exit
	delay_ms(120);
}

//LCD Init For 1.44Inch LCD Panel with ST7735R.
void Lcd_Init(void)
{
	osMutexAcquire(mutex_spi1,osWaitForever);
	Lcd_Reset(); //Reset before LCD Init.

	//LCD Init For 1.44Inch LCD Panel with ST7735R.
	Lcd_SleepOut();

	Lcd_WriteIndex(0x36); //MX, MY, RGB mode
	Lcd_WriteData(0x00);  //C0/00/A0/60   C8/68/A8/08

	//ST7735R Frame Rate
	Lcd_WriteIndex(0xB1); 
	Lcd_WriteData(0x01); 
	Lcd_WriteData(0x2C); 
	Lcd_WriteData(0x2D); 

	Lcd_WriteIndex(0xB2); 
	Lcd_WriteData(0x01); 
	Lcd_WriteData(0x2C); 
	Lcd_WriteData(0x2D); 

	Lcd_WriteIndex(0xB3); 
	Lcd_WriteData(0x01); 
	Lcd_WriteData(0x2C); 
	Lcd_WriteData(0x2D); 
	Lcd_WriteData(0x01); 
	Lcd_WriteData(0x2C); 
	Lcd_WriteData(0x2D); 
	
	Lcd_WriteIndex(0xB4); //Column inversion 
	Lcd_WriteData(0x07); 
	
	//ST7735R Power Sequence
	Lcd_WriteIndex(0xC0); 
	Lcd_WriteData(0xA2); 
	Lcd_WriteData(0x02); 
	Lcd_WriteData(0x84); 
	Lcd_WriteIndex(0xC1); 
	Lcd_WriteData(0xC5); 

	Lcd_WriteIndex(0xC2); 
	Lcd_WriteData(0x0A); 
	Lcd_WriteData(0x00); 

	Lcd_WriteIndex(0xC3); 
	Lcd_WriteData(0x8A); 
	Lcd_WriteData(0x2A); 
	Lcd_WriteIndex(0xC4); 
	Lcd_WriteData(0x8A); 
	Lcd_WriteData(0xEE); 
	
	Lcd_WriteIndex(0xC5); //VCOM 
	Lcd_WriteData(0x0E); 

	//ST7735R Gamma Sequence
	Lcd_WriteIndex(0xe0); 
	Lcd_WriteData(0x0f); 
	Lcd_WriteData(0x1a); 
	Lcd_WriteData(0x0f); 
	Lcd_WriteData(0x18); 
	Lcd_WriteData(0x2f); 
	Lcd_WriteData(0x28); 
	Lcd_WriteData(0x20); 
	Lcd_WriteData(0x22); 
	Lcd_WriteData(0x1f); 
	Lcd_WriteData(0x1b); 
	Lcd_WriteData(0x23); 
	Lcd_WriteData(0x37); 
	Lcd_WriteData(0x00); 	
	Lcd_WriteData(0x07); 
	Lcd_WriteData(0x02); 
	Lcd_WriteData(0x10); 

	Lcd_WriteIndex(0xe1); 
	Lcd_WriteData(0x0f); 
	Lcd_WriteData(0x1b); 
	Lcd_WriteData(0x0f); 
	Lcd_WriteData(0x17); 
	Lcd_WriteData(0x33); 
	Lcd_WriteData(0x2c); 
	Lcd_WriteData(0x29); 
	Lcd_WriteData(0x2e); 
	Lcd_WriteData(0x30); 
	Lcd_WriteData(0x30); 
	Lcd_WriteData(0x39); 
	Lcd_WriteData(0x3f); 
	Lcd_WriteData(0x00); 
	Lcd_WriteData(0x07); 
	Lcd_WriteData(0x03); 
	Lcd_WriteData(0x10);  

	//x地址
	Lcd_WriteIndex(0x2a);
	Lcd_WriteData(0x00); //偏移了2
	Lcd_WriteData(0x00);
	Lcd_WriteData(0x00);
	Lcd_WriteData(0x7f);

	//y地址
	Lcd_WriteIndex(0x2b);
	Lcd_WriteData(0x00);
	Lcd_WriteData(0x00);
	Lcd_WriteData(0x00);
	Lcd_WriteData(0x9f);
	
	Lcd_WriteIndex(0xF0); //Enable test command  
	Lcd_WriteData(0x01); 
	Lcd_WriteIndex(0xF6); //Disable ram power save mode 
	Lcd_WriteData(0x00); 
	
	Lcd_WriteIndex(0x3A); //65k mode 
	Lcd_WriteData(0x05); //16-bit RGB565

	Lcd_SleepOut();
	Lcd_WriteIndex(0x29);//Display on
	osMutexRelease(mutex_spi1);
}


/*************************************************
函数名：LCD_Set_Region
功能：设置lcd显示区域，在此区域写点数据自动换行
入口参数：xy起点和终点
返回值：无
*************************************************/
void Lcd_SetRegion(u16 x_start,u16 y_start,u16 x_end,u16 y_end)
{

	osMutexAcquire(mutex_spi1,osWaitForever);
	Lcd_WriteIndex(0x2A);
	Lcd_WriteData(0x00);
	//Lcd_WriteData(x_start+2);//Lcd_WriteData(x_start+2);
	Lcd_WriteData(x_start+2);//Lcd_WriteData(x_start+2);
	Lcd_WriteData(0x00);
	//Lcd_WriteData(x_end+2);
	Lcd_WriteData(x_end+2);

	Lcd_WriteIndex(0x2B);
	Lcd_WriteData(0x00);
	Lcd_WriteData(y_start+1);
	Lcd_WriteData(0x00);
	Lcd_WriteData(y_end+1);

	Lcd_WriteIndex(0x2C);

}

/*************************************************
函数名：LCD_Set_XY
功能：设置lcd显示起始点
入口参数：xy坐标
返回值：无
*************************************************/
void Lcd_SetXY(u16 x,u16 y)
{
  	Lcd_SetRegion(x,y,x,y);
	osMutexRelease(mutex_spi1);
}

	
/*************************************************
函数名：LCD_DrawPoint
功能：画一个点
入口参数：无
返回值：无
*************************************************/
void Lcd_DrawPoint(u16 x,u16 y,u16 Data)
{
	Lcd_SetRegion(x,y,x+1,y+1);
	Lcd_WriteData_16Bit(Data);
	osMutexRelease(mutex_spi1);

}    

/*****************************************
 函数功能：读TFT某一点的颜色                          
 出口参数：color  点颜色值                                 
******************************************/
unsigned int Lcd_ReadPoint(u16 x,u16 y)
{
  unsigned int Data;
  Lcd_SetXY(x,y);

  //Lcd_ReadData();//丢掉无用字节
  //Data=Lcd_ReadData();
  Lcd_WriteData(Data);
	osMutexRelease(mutex_spi1);

  return Data;
}
/*************************************************
函数名：Lcd_Clear
功能：全屏清屏函数
入口参数：填充颜色COLOR
返回值：无
*************************************************/
void Lcd_Clear(u16 Color)               
{
   volatile u16 i=0,m=0;
   Lcd_SetRegion(0,0,X_MAX_PIXEL-1,Y_MAX_PIXEL-1);
   Lcd_WriteIndex(0x2C);
   for(i=0;i<X_MAX_PIXEL;i++)
    for(m=0;m<Y_MAX_PIXEL;m++)
    {	
	  	Lcd_WriteData_16Bit(Color);
    }
	osMutexRelease(mutex_spi1);
}
void Lcd_Fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint16_t color)
{
	volatile uint16_t i = 0, j = 0;
	Lcd_SetRegion(xsta, ysta, xend - 1, yend - 1); //设置显示范围
	for (i = ysta; i < yend; i++)
	{
		for (j = xsta; j < xend; j++)
		{
			Lcd_WriteData_16Bit(color);
		}
	}
	osMutexRelease(mutex_spi1);

}
u16 LCD_BGR2RGB(u16 c)
{
  u16  r,g,b,rgb;
  b=(c>>0)&0x1f;
  g=(c>>5)&0x3f;
  r=(c>>11)&0x1f;
  rgb=(b<<11)+(g<<5)+(r<<0);
  return(rgb);

}
void lcd_set_light(u32 comp)
{
	TIM3->CCR1=comp;   //设置tim1ch1通道
}

