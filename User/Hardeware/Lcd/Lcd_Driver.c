//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ���������ɣ��������������κ���;
//  ��������   : 1.8��LCD 4�ӿ���ʾ����(STM32ϵ��)
/******************************************************************************
//������������STM32F103C8
//              GND   ��Դ��
//              VCC   ��5V��3.3v��Դ
//              SCL   ��PA5��SCL��
//              SDA   ��PA7��SDA��
//              RES   ��PB0
//              DC    ��PB1
//              CS    ��PA4 
//							BL		��PB10
*******************************************************************************/
#include "Lcd_Driver.h"
#include "spi.h"
#include "cmsis_os2.h"
#include "delay.h"
#include "MyTask.h"
#include "spi_sel.h"

//Һ��IO��ʼ������
void LCD_GPIO_Init(void)
{
	// __HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	//��ʼ��gpio
	GPIO_InitTypeDef gpio_initer;
	gpio_initer.Pin=GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
	gpio_initer.Mode=GPIO_MODE_OUTPUT_PP;
	//gpio_initer.Pull=GPIO_PULLUP;
	gpio_initer.Speed=GPIO_SPEED_HIGH;
	HAL_GPIO_Init(GPIOB,&gpio_initer);


}
//向SPI总线发送一个8位数据
void  SPI_WriteData(u8 data)
{
	HAL_SPI_Transmit(&spi_handler_1,&data,1,10);
}

//��Һ����дһ��8λָ��
void Lcd_WriteIndex(u8 Index)
{
   //SPI д����ʱ��ʼ
	SPI1_Sel_Device(SPI1_TFT);
	LCD_RS_CLR;
	 SPI_WriteData(Index);
	SPI1_Unsel_Device(SPI1_TFT);
}

void Lcd_WriteIndex_Lock(u8 Index)
{
	osMutexAcquire(mutex_spi1,osWaitForever);
	//SPI д����ʱ��ʼ
	SPI1_Sel_Device(SPI1_TFT);
	LCD_RS_CLR;
	SPI_WriteData(Index);
	SPI1_Unsel_Device(SPI1_TFT);
	osMutexRelease(mutex_spi1);
}

//��Һ����дһ��8λ����
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

//��Һ����дһ��16λ����
void Lcd_WriteData_16Bit(u16 Data)
{
	SPI1_Sel_Device(SPI1_TFT);
   LCD_RS_SET;
	 SPI_WriteData(Data>>8); 	//д���8λ����
	 SPI_WriteData(Data); 			//д���8λ����
	SPI1_Unsel_Device(SPI1_TFT);
}

void Lcd_WriteData_16Bit_Lock(u16 Data)
{
	osMutexAcquire(mutex_spi1,osWaitForever);
	SPI1_Sel_Device(SPI1_TFT);
	LCD_RS_SET;
	SPI_WriteData(Data>>8); 	//д���8λ����
	SPI_WriteData(Data); 			//д���8λ����
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

	//x��ַ
	Lcd_WriteIndex(0x2a);
	Lcd_WriteData(0x00); //ƫ����2
	Lcd_WriteData(0x00);
	Lcd_WriteData(0x00);
	Lcd_WriteData(0x7f);

	//y��ַ
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
��������LCD_Set_Region
���ܣ�����lcd��ʾ�����ڴ�����д�������Զ�����
��ڲ�����xy�����յ�
����ֵ����
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
��������LCD_Set_XY
���ܣ�����lcd��ʾ��ʼ��
��ڲ�����xy����
����ֵ����
*************************************************/
void Lcd_SetXY(u16 x,u16 y)
{
  	Lcd_SetRegion(x,y,x,y);
	osMutexRelease(mutex_spi1);
}

	
/*************************************************
��������LCD_DrawPoint
���ܣ���һ����
��ڲ�������
����ֵ����
*************************************************/
void Lcd_DrawPoint(u16 x,u16 y,u16 Data)
{
	Lcd_SetRegion(x,y,x+1,y+1);
	Lcd_WriteData_16Bit(Data);
	osMutexRelease(mutex_spi1);

}    

/*****************************************
 �������ܣ���TFTĳһ�����ɫ                          
 ���ڲ�����color  ����ɫֵ                                 
******************************************/
unsigned int Lcd_ReadPoint(u16 x,u16 y)
{
  unsigned int Data;
  Lcd_SetXY(x,y);

  //Lcd_ReadData();//���������ֽ�
  //Data=Lcd_ReadData();
  Lcd_WriteData(Data);
	osMutexRelease(mutex_spi1);

  return Data;
}
/*************************************************
��������Lcd_Clear
���ܣ�ȫ����������
��ڲ����������ɫCOLOR
����ֵ����
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
	Lcd_SetRegion(xsta, ysta, xend - 1, yend - 1); //������ʾ��Χ
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
	TIM3->CCR1=comp;   //����tim1ch1ͨ��
}

