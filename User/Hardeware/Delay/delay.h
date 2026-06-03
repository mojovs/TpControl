//
// Created by Meng on 2025/4/10.
//

#ifndef DELAY_H
#define DELAY_H
#include "stm32f1xx_hal.h"

void DWT_Delay_Init(void);
void delay_us(uint32_t us);
void delay_ms(uint16_t nms);

#endif //DELAY_H

