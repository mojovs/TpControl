#include "MyTask.h"

#include <stdio.h>

#include "main.h"
#include "cmsis_os2.h"
#include "gpio.h"
#include "tim.h"
#include "led.h"
#include "adc.h"
#include "ble.h"
#include "Gui.h"
#include "xprintf.h"
#include "delay.h"
#include "buzz.h"
#include "Humidity.h"
#include "queue.h"
#include "rtc.h"
#include "app_config.h"

//主程
osThreadId_t gui_task_hdl;
const osThreadAttr_t gui_Task_attributes = {
    .name = "gui_task_hdl",
    .stack_size = 2048,
    .priority = (osPriority_t)osPriorityLow,
};
osThreadId_t rtc_task_hdl;
const osThreadAttr_t rtc_Task_attributes = {
    .name = "rtc_task_hdl",
    .stack_size = 1024,
    .priority = (osPriority_t)osPriorityLow+2,
};
osThreadId_t lcdLight_task_hdl;
const osThreadAttr_t lcdLight_Task_attributes = {
    .name = "lcdLight_task_hdl",
    .stack_size = 128,
    .priority = (osPriority_t)osPriorityLow,
};
osThreadId_t humidity_ac_task_hdl;
const osThreadAttr_t humidity_ac_Task_attributes = {
    .name = "humidity_ac_task_hdl",
    .stack_size = 128,
    .priority = (osPriority_t)osPriorityLow2,
};

/*******************************/
/******锁******************/

osMutexId_t mutex_task;
const osMutexAttr_t mutex_task_attr = {
    .name = "Mutex_task", // 互斥量名称，调试用
    //.attr_bits = osMutexRecursive // 可选：设置为递归互斥量（允许同一任务多次加锁）
};
osMutexId_t mutex_spi1;
const osMutexAttr_t mutex_spi1_attr = {
    .name = "Mutex_spi1", // 互斥量名称，调试用
    .attr_bits = osMutexRecursive // SPI1 总线锁允许 littlefs 与底层 Flash 驱动递归获取
};
osMutexId_t mutex_lfs;
const osMutexAttr_t mutex_lfs_attr = {
    .name = "Mutex_lfs", // 互斥量名称，调试用
    .attr_bits = osMutexRecursive // 可选：设置为递归互斥量（允许同一任务多次加锁）
};

osMutexId_t mutex_hum;
const osMutexAttr_t mutex_hum_attr = {
    .name = "Mutex_hum", // 互斥量名称，调试用
    //.attr_bits = osMutexRecursive // 可选：设置为递归互斥量（允许同一任务多次加锁）
};
osMutexId_t mutex_tft;
const osMutexAttr_t mutex_tft_attr = {
    .name = "Mutex_tft", // 互斥量名称，调试用
    //.attr_bits = osMutexRecursive // 可选：设置为递归互斥量（允许同一任务多次加锁）
};

osSemaphoreId_t rtc_sema;
osSemaphoreAttr_t rtc_sema_attr={
    .name="sema rtc",
};




/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void)
{


    //创建mutex
    mutex_spi1 = osMutexNew(&mutex_spi1_attr);
    mutex_lfs= osMutexNew(&mutex_lfs_attr);

    mutex_task = osMutexNew(&mutex_task_attr);
    mutex_tft = osMutexNew(&mutex_tft_attr);
    //创建队列
    rtc_sema=osSemaphoreNew(1,0,&rtc_sema_attr);

    gui_task_hdl = osThreadNew(guiTask, NULL, &gui_Task_attributes);
    lcdLight_task_hdl = osThreadNew(lcdLightTask, NULL, &lcdLight_Task_attributes);
    humidity_ac_task_hdl = osThreadNew(humidityACTask, NULL, &humidity_ac_Task_attributes);
    rtc_task_hdl= osThreadNew(rtcTask, NULL, &rtc_Task_attributes);
}

void rtcTask(void* arg)
{
    for (;;)
    {
        osSemaphoreAcquire(rtc_sema, osWaitForever);
        rtc_get_time();
        osThreadFlagsSet(gui_task_hdl,1);
        // ble_send("%04d-%02d-%02d %01d %02d:%02d:%02d\n",calendar.year,calendar.month,calendar.date,calendar.week,calendar.hour,calendar.min,calendar.sec);



    }
}
void guiTask(void* argument)
{
    u8 update_static=0;
    for (;;)
    {
        if (update_static==0){
            //printf("Hello\r\n");
            Gui_showimage_12x12("/images/days.bin", 0, 0); //运行天数
            Gui_showimage_12x12("/images/fire.bin", 30, 0); //当前温度
            Gui_showimage_12x12("/images/buzz.bin", 88, 0); //当前喇叭状态
            Gui_showimage_12x12("/images/ble.bin", 102, 0); //蓝牙状态
            Gui_showimage_12x12("/images/alarm1.bin", 116, 0); //n闹钟
            Gui_showimage_12x12("/images/humidity.bin", 0, 15); //湿度状态
            Gui_showimage_12x12("/images/fan.bin", 36, 15); //风扇状态

            //
            Gui_DrawLine(0, 97, 128, 97,GRAY);
            Gui_DrawLine(0, 124, 128, 124,GRAY);
            Gui_showimage_24x24("/images/chicken.bin", 0, 100); //保小鸡模式
            Gui_showimage_24x24("/images/egg.bin", 24, 100); //鸡蛋模式

            // osDelay(800);
            Gui_showimage_12x12("/images/alarm2.bin", 116, 0); //闹铃抖动
            Gui_showimage_12x12("/images/alarm1.bin", 0, 133); //设置闹钟时间
            Gui_showimage_12x12("/images/clock.bin", 0, 147); //时间
            update_static=1;
             }
        uint32_t rtc_flag=osThreadFlagsWait(0x01,osFlagsWaitAny,osWaitForever);
        if (( rtc_flag&0x1)  !=0)  {
            print_string_gui(13,148,"%02d-%02d-%02d %01d %02d:%02d:%02d\n",calendar.year-2000,calendar.month,calendar.date,calendar.week,calendar.hour,calendar.min,calendar.sec);
        }
    }
    /* USER CODE END StartDefaultTask */
}

void lcdLightTask(void* argument)
{
    while (1)
    {
        led_on();
        osDelay(1000);
        led_off();
        osDelay(1000);
    }
}

void humidityACTask(void* argument)
{
    humidity_start();
    __HAL_TIM_CLEAR_FLAG(&tim1_handler, TIM_FLAG_UPDATE); // 清 UIF
    HAL_TIM_Base_Start_IT(&tim1_handler); //开始中断
    //  print_string_gui(0, 0, "humidity:%.2f KR",0.0);
    while (1)
    {
        u32 sum = 0, flag = 0;
        u16 tmp = 0;

        for (int i = 0; i < 3; i++)
        {
            flag = osThreadFlagsWait(0x01,osFlagsWaitAll,osWaitForever);
            if (flag & 0x1)
            {
                HAL_ADC_Start(&adc2_handler);
                tmp = humidity_get_value_single();
                sum += tmp;
            }
        }
        tmp = sum / 3;
        //
        //vol=tmp/4096*3.3
        //3.3*27/(R+27)=vol

        float res = ((float)4096 / tmp - 1) * 27;

        osDelay(100);
    }
}


void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName)
{
    // 堆栈溢出检测到
    led_shine(5, 3);

    // 你可以添加重启或异常处理逻辑
    while (1);
}


/* USER CODE END Application */
