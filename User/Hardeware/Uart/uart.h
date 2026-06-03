//
// Created by meng on 2024/6/12.
//

#ifndef LIGHT_IP5303_UART_H
#define LIGHT_IP5303_UART_H

#ifndef CLOCK_UART_H
#define CLOCK_UART_H
#include "stm32f1xx.h"
//#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_uart.h"
#include "app_config.h"

#if ENABLE_UART1_DEBUG
extern UART_HandleTypeDef  uart_handle;
void uart1_init(uint32_t bound);
int uart1_read_byte(uint8_t *ch, uint32_t timeout_ms);
int uart1_read(uint8_t *buf, uint32_t len, uint32_t timeout_ms);
void uart1_send(const uint8_t *buf, uint32_t len);
void uart1_printf(char *fmt, ...);
#endif


#endif //CLOCK_UART_H



#endif //LIGHT_IP5303_UART_H
