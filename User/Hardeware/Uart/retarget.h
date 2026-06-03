//
// Created by meng on 2024/6/12.
//

#ifndef LIGHT_IP5303_RETARGET_H
#define LIGHT_IP5303_RETARGET_H

#include "stm32f1xx_hal.h"
#include <sys/stat.h>
#include <stdio.h>
#include "stm32f1xx_hal_uart.h"

void RetargetInit(UART_HandleTypeDef *huart);

int _isatty(int fd);

int _write(int fd, char *ptr, int len);

int _close(int fd);

int _lseek(int fd, int ptr, int dir);

int _read(int fd, char *ptr, int len);

int _fstat(int fd, struct stat *st);
#endif //LIGHT_IP5303_RETARGET_H