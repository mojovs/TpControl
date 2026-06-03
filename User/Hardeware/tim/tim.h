//
// Created by meng on 2024/6/7.
//

#ifndef LIGHT_IP5303_TIM_H
#define LIGHT_IP5303_TIM_H

#include "cmsis_os.h"
#include "cmsis_os2.h"
#include "sys.h"
/**
 * PA0:用来检测信号，一但检测到信号，将检测标志置1，打断平时的工作状态
 * PA1：用来给电源芯片 供PWM
 * PA2：PWM控制灯
 * PA6：控制指示灯
 */

extern u8 TIM2CH1_CAPTURE_STA; //输入捕获状态
extern u32 TIM2CH1_CAPTURE_VAL; //输入捕获值(TIM2/TIM5是32位)


extern TIM_HandleTypeDef tim1_handler;
extern TIM_OC_InitTypeDef tim1_oc_initer;

extern TIM_HandleTypeDef tim2_handler;
extern TIM_IC_InitTypeDef tim2_ic_initer;

extern TIM_HandleTypeDef tim3_handler;
extern TIM_OC_InitTypeDef tim3_oc_initer;
/// PA8:TIM1CH1 灯PWM
void tim1_init();
/// PA0： TIM2CH1 读取BISS0001
void tim2_init();
/// PA6 IP5303定时
void tim3_init();
// void tim3_init();
/**
* @brief  捕获PA0引脚的定时器溢出中断
* @return
*/
void tim2_ch1_cap_elpased();
void tim2_ch1_caped();
/**
 * 人体检测电路信号的脉冲宽度。
 * @param temp 检测到的PA0,定时器次数
 */
void tim2ch1_cap_output(u32 temp);

#endif //LIGHT_IP5303_TIM_H
