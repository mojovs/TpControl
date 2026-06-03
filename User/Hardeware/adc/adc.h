//
// Created by Meng on 2024/7/9.
//
#ifndef M_ADC_H
#define M_ADC_H

#include "sys.h"

extern ADC_HandleTypeDef adc2_handler;
void adc2_init();

//获取adc 2  ch1的值
void humidity_start(void);

/**
 * 获取adc2 ch1 的值
 * @return adc值
 */
u16 humidity_get_value_single(void);
/**
 * 光线传感器检测平均值
 * @param times 捕获次数
 * @return 平均值
 */
void adc_get_humidity_average(u16 times);
/**
 * 人体检测传感器平均值
 * @param times 捕获次数
 * @return 平均值
 */
u16 humidity_sensor_get_average();

float humidity_sensor_get_voltage();


#endif

