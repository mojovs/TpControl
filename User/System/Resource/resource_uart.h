//
// Created by Meng on 2026/6/2.
//

#ifndef RESOURCE_UART_H
#define RESOURCE_UART_H

#include "cmsis_os2.h"
#include "app_config.h"

#if ENABLE_RESOURCE_UART
void resourceUartTask(void *argument);
void resource_uart_poll_command(void);
#endif

#endif //RESOURCE_UART_H
