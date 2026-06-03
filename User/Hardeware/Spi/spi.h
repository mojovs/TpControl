//
// Created by Meng on 2024/7/14.
//

#ifndef SPI_H
#define SPI_H
#include "sys.h"
#include "stm32f1xx_hal_spi.h"
extern SPI_HandleTypeDef spi_handler_1;
void spi1_init();   //初始化spi0
void spi1_set_speed(u8 speed);

#endif //SPI_H

