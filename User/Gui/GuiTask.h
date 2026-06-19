//
// Created by Meng on 2026/6/12.
//

#ifndef GUITASK_H
#define GUITASK_H

#include "sys.h"
#include "cmsis_os2.h"

/*========= 场景函数声明 =========*/

/**
 * @brief guiTask 任务入口（FreeRTOS 线程）
 */
void guiTask(void *argument);

/**
 * @brief 主界面静态绘制（缓存场景，绘制不变部分）
 */
void scene_Main_Static(void);

/**
 * @brief 主界面动态刷新（每秒更新温度/湿度/运行时长）
 */
void scene_Main_Dynamic(void);

void scene_Mode_Static(void);
void scene_Mode_Dynamic(void);
void scene_Setting_Static(void);
void scene_Setting_Dynamic(void);

#endif //GUITASK_H