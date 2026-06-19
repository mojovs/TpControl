/**
 * @file    control.c
 * @brief   温湿度控制模块实现
 *
 * 控制策略: 滞回比较 (On/Off)
 *   加热: T < (target - 0.5°C) → 开启;  T > (target + 0.5°C) → 关闭
 *   风扇: H > (target + 3%RH)  → 开启;  H < (target - 3%RH)  → 关闭
 *
 * 注意: 实际硬件确认前, 继电器初始化会闪烁加热+风扇各一次作为自检。
 */

#include "control.h"

#include "delay.h"

/*========= 滞回参数 =========*/

/** @brief 温度滞回半宽 (x10, 5 = 0.5°C) */
#define TEMP_HYST_X10       5

/** @brief 湿度滞回半宽 (x10, 30 = 3.0%RH) */
#define HUM_HYST_X10        30

/*========= 全局变量定义 =========*/

int16_t g_target_temp_x10 = 375;    /* 默认 37.5°C */
int16_t g_target_hum_x10  = 650;    /* 默认 65.0%RH */
volatile uint8_t g_heater_on = 0;
volatile uint8_t g_fan_on    = 0;

/*========= 继电器 GPIO 初始化和控制 =========*/

void control_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_MEDIUM;
    gpio.Pull  = GPIO_PULLDOWN;        /* 默认低电平, 继电器断开 */

    /* 加热继电器 */
    gpio.Pin = HEATER_PIN;
    HAL_GPIO_Init(HEATER_PORT, &gpio);
    HAL_GPIO_WritePin(HEATER_PORT, HEATER_PIN, GPIO_PIN_RESET);

    /* 风扇继电器 */
    gpio.Pin = FAN_PIN;
    HAL_GPIO_Init(FAN_PORT, &gpio);
    HAL_GPIO_WritePin(FAN_PORT, FAN_PIN, GPIO_PIN_RESET);

    /* 自检: 闪烁各一次 */
    control_heater_set(1);
    delay_ms(100);
    control_heater_set(0);
    delay_ms(50);
    control_fan_set(1);
    delay_ms(100);
    control_fan_set(0);
}

void control_heater_set(uint8_t on)
{
    if (on)
        HAL_GPIO_WritePin(HEATER_PORT, HEATER_PIN, GPIO_PIN_SET);
    else
        HAL_GPIO_WritePin(HEATER_PORT, HEATER_PIN, GPIO_PIN_RESET);
    g_heater_on = on;
}

void control_fan_set(uint8_t on)
{
    if (on)
        HAL_GPIO_WritePin(FAN_PORT, FAN_PIN, GPIO_PIN_SET);
    else
        HAL_GPIO_WritePin(FAN_PORT, FAN_PIN, GPIO_PIN_RESET);
    g_fan_on = on;
}

void control_set_targets(int16_t temp_x10, int16_t hum_x10)
{
    if (temp_x10 >= 0)   g_target_temp_x10 = temp_x10;
    if (hum_x10  >= 0)   g_target_hum_x10  = hum_x10;
}

void control_tick(int16_t cur_temp_x10, int16_t cur_hum_x10)
{
    /* ---- 温度控制 (滞回) ---- */
    if (cur_temp_x10 < g_target_temp_x10 - TEMP_HYST_X10)
    {
        if (!g_heater_on) control_heater_set(1);
    }
    else if (cur_temp_x10 > g_target_temp_x10 + TEMP_HYST_X10)
    {
        if (g_heater_on) control_heater_set(0);
    }
    /* 滞回区内: 保持当前状态不变 */

    /* ---- 湿度控制 (滞回) ---- */
    if (cur_hum_x10 > g_target_hum_x10 + HUM_HYST_X10)
    {
        if (!g_fan_on) control_fan_set(1);
    }
    else if (cur_hum_x10 < g_target_hum_x10 - HUM_HYST_X10)
    {
        if (g_fan_on) control_fan_set(0);
    }
    /* 滞回区内: 保持当前状态不变 */
}
