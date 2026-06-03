//
// Created by Meng on 2026/6/2.
//

#include "ymodem.h"

#include "app_config.h"

#if ENABLE_RESOURCE_UART && ENABLE_RESOURCE_YMODEM

#include <string.h>

#include "stm32f1xx_hal.h"
#include "uart.h"

#define YMODEM_SOH        0x01
#define YMODEM_STX        0x02
#define YMODEM_EOT        0x04
#define YMODEM_ACK        0x06
#define YMODEM_NAK        0x15
#define YMODEM_CAN        0x18
#define YMODEM_CRC        'C'

#define YMODEM_PACKET_128   128
#define YMODEM_PACKET_1K    1024
#define YMODEM_TIMEOUT_MS   3000
#define YMODEM_MAX_RETRY    10
#define YMODEM_NAME_SIZE    64

static uint8_t ymodem_packet_buf[YMODEM_PACKET_1K];

static uint16_t ymodem_crc16(const uint8_t *data, uint32_t len)
{
    uint16_t crc = 0;
    uint32_t i;
    uint8_t bit;

    for (i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (bit = 0; bit < 8; bit++) {
            if (crc & 0x8000) {
                crc = (uint16_t)((crc << 1) ^ 0x1021);
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

static void ymodem_send_byte(uint8_t ch)
{
    uart1_send(&ch, 1);
}

static int ymodem_read_byte(uint8_t *ch, uint32_t timeout_ms)
{
    return uart1_read_byte(ch, timeout_ms);
}

static int ymodem_parse_size(const char *str, uint32_t *size)
{
    uint32_t value = 0;

    if (*str == '\0') {
        return 0;
    }

    while (*str && *str != ' ') {
        if (*str < '0' || *str > '9') {
            return 0;
        }
        value = value * 10 + (uint32_t)(*str - '0');
        str++;
    }

    *size = value;
    return 1;
}

static int ymodem_read_packet(uint8_t first, uint8_t *seq, uint32_t *packet_size)
{
    uint8_t seq_inv;
    uint8_t crc_hi;
    uint8_t crc_lo;
    uint16_t crc_rx;
    uint16_t crc_calc;
    uint32_t i;

    if (first == YMODEM_SOH) {
        *packet_size = YMODEM_PACKET_128;
    } else if (first == YMODEM_STX) {
        *packet_size = YMODEM_PACKET_1K;
    } else {
        return YMODEM_ERR_PACKET;
    }

    if (!ymodem_read_byte(seq, YMODEM_TIMEOUT_MS)) {
        return YMODEM_ERR_TIMEOUT;
    }
    if (!ymodem_read_byte(&seq_inv, YMODEM_TIMEOUT_MS)) {
        return YMODEM_ERR_TIMEOUT;
    }
    if (((*seq) + seq_inv) != 0xFF) {
        return YMODEM_ERR_PACKET;
    }

    for (i = 0; i < *packet_size; i++) {
        if (!ymodem_read_byte(&ymodem_packet_buf[i], YMODEM_TIMEOUT_MS)) {
            return YMODEM_ERR_TIMEOUT;
        }
    }

    if (!ymodem_read_byte(&crc_hi, YMODEM_TIMEOUT_MS)) {
        return YMODEM_ERR_TIMEOUT;
    }
    if (!ymodem_read_byte(&crc_lo, YMODEM_TIMEOUT_MS)) {
        return YMODEM_ERR_TIMEOUT;
    }

    crc_rx = ((uint16_t)crc_hi << 8) | crc_lo;
    crc_calc = ymodem_crc16(ymodem_packet_buf, *packet_size);
    if (crc_rx != crc_calc) {
        return YMODEM_ERR_PACKET;
    }

    return YMODEM_OK;
}

static int ymodem_handle_header(uint32_t packet_size,
                                ymodem_file_begin_cb_t begin,
                                void *ctx,
                                uint32_t *file_size)
{
    char name[YMODEM_NAME_SIZE];
    char *size_str;
    uint32_t i;

    (void)packet_size;

    if (ymodem_packet_buf[0] == '\0') {
        return YMODEM_ERR_EMPTY;
    }

    for (i = 0; i < sizeof(name) - 1 && ymodem_packet_buf[i] != '\0'; i++) {
        name[i] = (char)ymodem_packet_buf[i];
    }
    name[i] = '\0';

    if (i == sizeof(name) - 1 && ymodem_packet_buf[i] != '\0') {
        return YMODEM_ERR_PACKET;
    }

    size_str = (char*)&ymodem_packet_buf[i + 1];
    if (!ymodem_parse_size(size_str, file_size)) {
        return YMODEM_ERR_PACKET;
    }

    if (begin(name, *file_size, ctx) != 0) {
        return YMODEM_ERR_CALLBACK;
    }

    return YMODEM_OK;
}

int ymodem_receive(ymodem_file_begin_cb_t begin,
                   ymodem_file_data_cb_t data,
                   ymodem_file_end_cb_t end,
                   void *ctx)
{
    uint8_t ch;
    uint8_t seq;
    uint8_t expected_seq = 1;
    uint8_t retry;
    uint8_t got_eot = 0;
    uint32_t packet_size;
    uint32_t file_size = 0;
    uint32_t received = 0;
    int ret;

    for (retry = 0; retry < YMODEM_MAX_RETRY; retry++) {
        ymodem_send_byte(YMODEM_CRC);
        if (!ymodem_read_byte(&ch, 1000)) {
            continue;
        }

        if (ch == YMODEM_CAN) {
            return YMODEM_ERR_CANCEL;
        }

        ret = ymodem_read_packet(ch, &seq, &packet_size);
        if (ret == YMODEM_OK && seq == 0) {
            ret = ymodem_handle_header(packet_size, begin, ctx, &file_size);
            if (ret == YMODEM_OK) {
                ymodem_send_byte(YMODEM_ACK);
                ymodem_send_byte(YMODEM_CRC);
                break;
            }
            if (ret == YMODEM_ERR_EMPTY) {
                ymodem_send_byte(YMODEM_ACK);
                return YMODEM_ERR_EMPTY;
            }
        }

        ymodem_send_byte(YMODEM_NAK);
    }

    if (retry >= YMODEM_MAX_RETRY) {
        return YMODEM_ERR_TIMEOUT;
    }

    while (1) {
        if (!ymodem_read_byte(&ch, YMODEM_TIMEOUT_MS)) {
            ymodem_send_byte(YMODEM_NAK);
            continue;
        }

        if (ch == YMODEM_CAN) {
            return YMODEM_ERR_CANCEL;
        }

        if (ch == YMODEM_EOT) {
            if (!got_eot) {
                got_eot = 1;
                ymodem_send_byte(YMODEM_NAK);
                continue;
            }

            ymodem_send_byte(YMODEM_ACK);
            ymodem_send_byte(YMODEM_CRC);

            if (!ymodem_read_byte(&ch, YMODEM_TIMEOUT_MS)) {
                return YMODEM_ERR_TIMEOUT;
            }

            ret = ymodem_read_packet(ch, &seq, &packet_size);
            if (ret == YMODEM_OK && seq == 0 && ymodem_packet_buf[0] == '\0') {
                ymodem_send_byte(YMODEM_ACK);
                if (end(ctx) != 0) {
                    return YMODEM_ERR_CALLBACK;
                }
                return YMODEM_OK;
            }

            return YMODEM_ERR_PACKET;
        }

        ret = ymodem_read_packet(ch, &seq, &packet_size);
        if (ret != YMODEM_OK) {
            ymodem_send_byte(YMODEM_NAK);
            continue;
        }

        if (seq == expected_seq) {
            uint32_t write_len = packet_size;

            if (received + write_len > file_size) {
                write_len = file_size - received;
            }

            if (write_len > 0 && data(ymodem_packet_buf, write_len, ctx) != 0) {
                ymodem_send_byte(YMODEM_CAN);
                ymodem_send_byte(YMODEM_CAN);
                return YMODEM_ERR_CALLBACK;
            }

            received += write_len;
            expected_seq++;
            ymodem_send_byte(YMODEM_ACK);
        } else if (seq == (uint8_t)(expected_seq - 1)) {
            ymodem_send_byte(YMODEM_ACK);
        } else {
            ymodem_send_byte(YMODEM_NAK);
        }
    }
}
#endif
