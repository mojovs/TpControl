//
// Created by Meng on 2026/6/2.
//

#ifndef YMODEM_H
#define YMODEM_H

#include <stdint.h>
#include "app_config.h"

#define YMODEM_OK              0
#define YMODEM_ERR_TIMEOUT    -1
#define YMODEM_ERR_CANCEL     -2
#define YMODEM_ERR_PACKET     -3
#define YMODEM_ERR_CALLBACK   -4
#define YMODEM_ERR_EMPTY      -5

#if ENABLE_RESOURCE_UART && ENABLE_RESOURCE_YMODEM
typedef int (*ymodem_file_begin_cb_t)(const char *name, uint32_t size, void *ctx);
typedef int (*ymodem_file_data_cb_t)(const uint8_t *data, uint32_t len, void *ctx);
typedef int (*ymodem_file_end_cb_t)(void *ctx);

int ymodem_receive(ymodem_file_begin_cb_t begin,
                   ymodem_file_data_cb_t data,
                   ymodem_file_end_cb_t end,
                   void *ctx);
#endif

#endif //YMODEM_H
