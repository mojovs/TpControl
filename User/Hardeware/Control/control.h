/**
 * @file    control.h
 * @brief   温湿度控制模块 — 目标值管理 + 继电器输出
 *
 * 硬件:
 *   PB12 — 加热继电器 (高电平闭合)
 *   PB13 — 风扇继电器 (高电平闭合)
 */

#ifndef CONTROL_H
#define CONTROL_H

#include "sys.h"

#ifdef __cplusplus
extern "C" {
#endif

/*========= 继电器引脚定义 =========*/

#define HEATER_PORT     GPIOB
#define HEATER_PIN      GPIO_PIN_12

#define FAN_PORT        GPIOB
#define FAN_PIN         GPIO_PIN_13

/*========= 全局目标值 =========*/

/** @brief 目标温度 (x10, 如 375 = 37.5°C) */
extern int16_t g_target_temp_x10;

/** @brief 目标湿度 (x10, 如 650 = 65.0%RH) */
extern int16_t g_target_hum_x10;

/*========= 控制状态 =========*/

/** @brief 加热继电器当前状态: 0=关, 1=开 */
extern volatile uint8_t g_heater_on;

/** @brief 风扇继电器当前状态: 0=关, 1=开 */
extern volatile uint8_t g_fan_on;

/*========= 接口函数 =========*/

/**
 * @brief 初始化继电器 GPIO (应在 osKernelStart 前调用)
 */
void control_init(void);

/**
 * @brief 设置加热继电器
 * @param on  0=断开, 1=闭合
 */
void control_heater_set(uint8_t on);

/**
 * @brief 设置风扇继电器
 * @param on  0=断开, 1=闭合
 */
void control_fan_set(uint8_t on);

/**
 * @brief 应用目标温湿度 (由 guiTask 在退出的设置界面时调用)
 * @param temp_x10  目标温度 x10 (如 375)
 * @param hum_x10   目标湿度 x10 (如 650)
 */
void control_set_targets(int16_t temp_x10, int16_t hum_x10);

/**
 * @brief 控制 tick — 读取当前值, 对比目标, 输出继电器
 *
 * 在每个 RTC 更新周期调用 (目前在 guiTask 的 RTC 中断处理中)。
 *
 * @param cur_temp_x10  当前温度 x10
 * @param cur_hum_x10   当前湿度 x10
 */
void control_tick(int16_t cur_temp_x10, int16_t cur_hum_x10);

#ifdef __cplusplus
}
#endif

#endif /* CONTROL_H */
