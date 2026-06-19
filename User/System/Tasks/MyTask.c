#include "MyTask.h"

#include <stdio.h>

#include "main.h"
#include "cmsis_os2.h"
#include "gpio.h"
#include "tim.h"
#include "led.h"
#include "adc.h"
#include "ble.h"
#include "GuiTask.h"
#include "delay.h"
#include "buzz.h"
#include "Humidity.h"
#include "queue.h"
#include "rtc.h"
#include "app_config.h"
#include "lfs.h"

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

static uint32_t s_start_sec = 0;        /* 系统启动时的RTC秒数 */
uint32_t g_elapsed_sec = 0;             /* 已运行秒数（供guiTask显示） */

/** @brief CJHR-31 实测电阻值 (kΩ × 100), humidityACTask 每秒更新 */
volatile uint32_t g_hum_res_ohm_x100 = 0;

void rtcTask(void* arg)
{
    for (;;)
    {
        osSemaphoreAcquire(rtc_sema, osWaitForever);
        rtc_get_time();

        // 记录启动时间（首次运行）
        if (s_start_sec == 0) {
            s_start_sec = rtc_date2sec(calendar.year, calendar.month, calendar.date,
                                        calendar.hour, calendar.min, calendar.sec);
        }
        // 计算已运行秒数
        uint32_t now_sec = rtc_date2sec(calendar.year, calendar.month, calendar.date,
                                         calendar.hour, calendar.min, calendar.sec);
        g_elapsed_sec = now_sec - s_start_sec;

        osThreadFlagsSet(gui_task_hdl, GUI_FLAG_RTC_UPDATE);
    }
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
        /*
         * 分压电路: 3.3V ─ CJHR ─┬─ 47kΩ ─ GND
         *                       PA4 (ADC)
         * V_47k = tmp / 4096 * 3.3
         * R_cjhr = (3.3 / V_47k - 1) * 47 (kΩ)
         */
        u32 sum = 0, valid_cnt = 0;
        u32 flag;

        for (int i = 0; i < 3; i++)
        {
            flag = osThreadFlagsWait(0x01, osFlagsWaitAll, osWaitForever);
            if (flag & 0x1)
            {
                HAL_ADC_Start(&adc2_handler);
                u16 tmp = humidity_get_value_single();

                /* 排除无效采样值 (ADC 12bit, 0=超时, ≥4095=饱和) */
                if (tmp > 0 && tmp < 4095) {
                    sum += tmp;
                    valid_cnt++;
                }
            }
        }

        if (valid_cnt > 0)
        {
            u16 tmp = (u16)(sum / valid_cnt);
            float res = 47.0f * (4096.0f / tmp - 1.0f);
            g_hum_res_ohm_x100 = (uint32_t)(res * 100.0f + 0.5f);
        }
        /* 无有效样本则保持上次值，避免界面跳变 */

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
