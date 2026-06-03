//
// Created by Meng on 2024/8/25.
//

#ifndef TEMPERATURE_H
#define TEMPERATURE_H
#include "sys.h"
#include "delay.h"
// 引脚输出
#define DS18B20_DQ_OUT(x) do{ x ? \
HAL_GPIO_WritePin(GPIOA,GPIO_PIN_0, GPIO_PIN_SET):\
HAL_GPIO_WritePin(GPIOA,GPIO_PIN_0, GPIO_PIN_RESET);\
}while(0)
/* 数据端口输出 */
#define DS18B20_DQ_IN HAL_GPIO_ReadPin(GPIOA, \
GPIO_PIN_0)/* 数据端口输入 */
u8 temperature_init();
//
void ds18b20_reset(void);

u8 ds18b20_check(void);

void ds18b20_write_byte(uint8_t data);

uint8_t ds18b20_read_bit(void);

uint8_t ds18b20_read_byte(void);
void ds18b20_start(void);
short ds18b20_get_temperature(void);
void show_termperature();


#endif //TEMPERATURE_H

