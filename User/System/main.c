#include "main.h"

#include <string.h>

#include "cmsis_os.h"
#include "gpio.h"
#include "sys.h"
#include "adc.h"
#include "uart.h"
#include "Lcd_Driver.h"
#include "GUI.h"
#include "spi.h"
#include "delay.h"
#include "W25Q80.h"
#include "led.h"
#include "tim.h"
#include "xprintf.h"
#include "buzz.h"
#include "lfs.h"
#include "w25q80_fs.h"
#include "Temperature.h"
#include "ble.h"
#include "Font.h"
#include "Humidity.h"
#include "Image.h"
#include "rtc.h"
#include "Key/key.h"
#include "Fs/w25q80_fs.h"
#include "app_config.h"
#include "MyTask.h"
#include "control.h"

#ifdef WRITE_CJHR31
#include "cjhr31_table_write.h"
#endif
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);

/**
  * @brief  The application entry point.
  * @retval int
  */

// void flash_test(void);
void lfs_test(void);
void temp_test(void);
void test_ble(void)
{
  while (1)
  {
    temp_test();
    delay_ms(1000);
  }
}


int main(void)
{
  // 在初始化之前添加这行代码，禁用JTAG，释放PB4
  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_AFIO_REMAP_SWJ_NOJTAG();
  //__HAL_AFIO_REMAP_SWJ_DISABLE(); //不能用，不然会导致 无法调试

  HAL_Init();

  Stm32_Clock_Init(RCC_PLL_MUL9);
  DWT_Delay_Init(); //精确延时

  osKernelInitialize();
  MX_FREERTOS_Init();
#if ENABLE_UART1_DEBUG
  uart1_init(115200); //USART1 普通调试串口
#endif
  ble_init(9600); //蓝牙
  ble_send("ble inited\r\n");
  rtc_init();
  spi1_init();
  ble_send("spi1 inited\r\n");
  w25q80_gpio_init();
  ble_send("flash inited\r\n");
  LCD_GPIO_Init();
  w25q80_init_fs(); //初始化文件系统

  uint8_t mid; uint16_t did;
  w25q80_read_id(&mid,&did);
// #ifdef WRITE_CJHR31
//   cjhr31_table_write_to_lfs();
// #endif


  lfs_list_dir("/");

  ble_send("File list all\r\n");
  Lcd_Init();
  delay_ms(100);
  Lcd_Init();
  Lcd_Clear(BLACK);
    // scene_Main_Static();
  // flash_test();
  tim3_init();
  key_init();
  lcd_set_light(10);  //设置lcd亮度
  buzz_init();
  tim1_init();  //定时器初始化
  adc2_init();  //初始化adc2 1
  humidity_init();  //初始化交流电源正负端 ble_init(9600);
  //flash_test();
  led_init();
  temperature_init();
  control_init();
  //write_image();
  osKernelStart();


  /* Start scheduler */
  while (1)
  {
    osDelay(1);
  }
  /* USER CODE END 3 */
}
void flash_test(void)
{
#if 0
  //设置 亮度
  u8 data[100]={0};
  u8 MID;
  u16 DID;
  u8 arry_w[]="hello";
  u8 arry_r[5];
  Gui_ShowString(0,0,"MID:",RED,WHITE,12,0);
  Gui_ShowString(0,12,"DID:",RED,WHITE,12,0);
  Gui_ShowString(0,24,"W:",RED,WHITE,12,0);
  Gui_ShowString(0,36,"R:",RED,WHITE,12,0);
  w25q80_read_id(&MID,&DID);

  xsprintf(data,"%d",MID);
  Gui_ShowString(48,0,data,RED,WHITE,12,0);
  memset(data,0,sizeof(data));

  xsprintf(data,"%d",DID);
  Gui_ShowString(48,12,data,RED,WHITE,12,0);
  memset(data,0,sizeof(data));

  w25q80_read_data(0x000000,arry_r,5);


  xsprintf(data,"%s",arry_r);
  Gui_ShowString(48,36,data,RED,WHITE,12,0);
  memset(data,0,sizeof(data));
#endif
  Gui_showimage_24x24("/images/bean_24x24.bin",0,0);
  // print_string_gui(25,12,WHITE,BLACK,"nadou");
  u8 buf[12]="纳豆模式";
  Gui_ShowChinese(25,12,buf,WHITE,BLACK,12,0);
}


void temp_test(void)
{
  u16 temp=0;
  temp=ds18b20_get_temperature();
  print_string_gui(0,0,"Temp:%d.%d",temp/10,temp%10);
  ble_send("当前温度:%d.%d\n",temp/10,temp%10);
}


/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{

  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{

}
#endif /* USE_FULL_ASSERT */
