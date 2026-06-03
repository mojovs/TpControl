//
// Created by Meng on 2025/4/6.
//

#ifndef MYTASK_H
#define MYTASK_H

#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"
#include "sys.h"
#include "queue.h"
#include "app_config.h"


extern osThreadId_t gui_task_hdl;
extern osThreadId_t humidity_ac_task_hdl;

 extern osMutexId_t mutex_task;
extern osMutexId_t mutex_spi1;
extern osMutexId_t mutex_lfs;  //给little fs用的锁

extern osMutexId_t mutex_tft;

extern osSemaphoreId_t rtc_sema;
void guiTask(void *argument);
void lcdLightTask(void *argument);
void humidityACTask(void *argument);
void sysTimerCallback(void *arg);
void rtcTask(void *arg);


void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName);   //堆栈溢出
#endif //MYTASK_H

