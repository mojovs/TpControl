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
#include "w25q80_fs.h"

extern lfs_t lfs;
extern struct lfs_config cfg_w25q80_fs;

extern osThreadId_t gui_task_hdl;
extern osThreadId_t humidity_ac_task_hdl;

#define GUI_FLAG_RTC_UPDATE     0x01U
#define GUI_FLAG_EC11_CW        0x02U
#define GUI_FLAG_EC11_CCW       0x04U
#define GUI_FLAG_EC11_PRESS     0x08U
#define GUI_FLAG_TIMER_TIMEOUT  0x10U
#define GUI_FLAG_ALL            (GUI_FLAG_RTC_UPDATE | GUI_FLAG_EC11_CW | GUI_FLAG_EC11_CCW | GUI_FLAG_EC11_PRESS)

/* 定时器超时全局标志 — 其他任务可轮询此变量 */
extern volatile uint8_t g_timer_timeout;

 extern osMutexId_t mutex_task;
extern osMutexId_t mutex_spi1;
extern osMutexId_t mutex_lfs;

extern osMutexId_t mutex_tft;

extern uint32_t g_elapsed_sec;

/** @brief CJHR-31 实测电阻值 (kΩ × 100), humidityACTask 每秒更新, guiTask cjhr31_calc_rh 查表用 */
extern volatile uint32_t g_hum_res_ohm_x100;

extern osSemaphoreId_t rtc_sema;
void guiTask(void *argument);
void lcdLightTask(void *argument);
void humidityACTask(void *argument);
void sysTimerCallback(void *arg);
void rtcTask(void *arg);

void MX_FREERTOS_Init(void);

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName);
#endif //MYTASK_H

