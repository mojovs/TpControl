//
// Created by Meng on 2025/11/25.
//

#ifndef BLE_H
#define BLE_H
#include "sys.h"
#include "stm32f1xx.h"
//#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_uart.h"
#include "stdarg.h"
#define BLE
static u8 send_data[512]={0};
extern UART_HandleTypeDef  ble_handle;
extern uint8_t  ble_rx_flag;
void ble_init(uint32_t bound);
void ble_send(char *fmt,...);
void ble_send_m(char *fmt,va_list arg);
void ble_receive(char *msg);



#endif //BLE_H

