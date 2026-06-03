//
// Created by Meng on 2026/6/2.
//

#include "resource_uart.h"

#include "app_config.h"

#if ENABLE_RESOURCE_UART

#include <string.h>

#include "lfs.h"
#include "uart.h"
#include "w25q80_fs.h"
#if ENABLE_RESOURCE_YMODEM
#include "ymodem.h"
#endif

#define RES_UART_LINE_SIZE      96
#define RES_UART_PATH_SIZE      64
#define RES_UART_CHUNK_SIZE     128
#define RES_UART_MAX_FILE_SIZE  (512UL * 1024UL)
#define RES_UART_READ_TIMEOUT   3000

#if ENABLE_RESOURCE_RAW_UPLOAD
static uint32_t resource_crc32_update(uint32_t crc, const uint8_t *data, uint32_t len)
{
    uint32_t i;
    uint8_t bit;

    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (bit = 0; bit < 8; bit++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320UL;
            } else {
                crc >>= 1;
            }
        }
    }

    return crc;
}

static int resource_hex_value(char ch)
{
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }
    if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    }
    return -1;
}

static int resource_parse_u32_dec(const char *str, uint32_t *value)
{
    uint32_t result = 0;

    if (*str == '\0') {
        return 0;
    }

    while (*str) {
        if (*str < '0' || *str > '9') {
            return 0;
        }
        result = result * 10 + (uint32_t)(*str - '0');
        str++;
    }

    *value = result;
    return 1;
}

static int resource_parse_u32_hex(const char *str, uint32_t *value)
{
    uint32_t result = 0;
    uint8_t digits = 0;

    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        str += 2;
    }

    while (*str) {
        int v = resource_hex_value(*str);
        if (v < 0) {
            return 0;
        }
        result = (result << 4) | (uint32_t)v;
        digits++;
        str++;
    }

    if (digits == 0 || digits > 8) {
        return 0;
    }

    *value = result;
    return 1;
}
#endif

static int resource_read_line(char *line, uint32_t max_len)
{
    uint32_t len = 0;
    uint8_t ch;

    while (len < max_len - 1) {
        if (!uart1_read_byte(&ch, HAL_MAX_DELAY)) {
            continue;
        }

        if (ch == '\r') {
            continue;
        }

        if (ch == '\n') {
            line[len] = '\0';
            return (int)len;
        }

        line[len++] = (char)ch;
    }

    line[len] = '\0';
    return (int)len;
}

static int resource_path_valid(const char *path)
{
    if (path[0] != '/') {
        return 0;
    }

    if (strlen(path) >= RES_UART_PATH_SIZE) {
        return 0;
    }

    if (strstr(path, "..") != 0) {
        return 0;
    }

    return 1;
}

static int resource_name_valid(const char *name)
{
    if (name[0] == '\0') {
        return 0;
    }

    if (strlen(name) >= RES_UART_PATH_SIZE) {
        return 0;
    }

    if (strchr(name, '/') != 0 || strchr(name, '\\') != 0) {
        return 0;
    }

    if (strstr(name, "..") != 0) {
        return 0;
    }

    return 1;
}

static int resource_join_path(char *out, uint32_t out_size, const char *dir, const char *name)
{
    uint32_t dir_len;
    uint32_t name_len;

    if (!resource_path_valid(dir) || !resource_name_valid(name)) {
        return 0;
    }

    dir_len = strlen(dir);
    name_len = strlen(name);

    if (strcmp(dir, "/") == 0) {
        if (1 + name_len >= out_size) {
            return 0;
        }
        out[0] = '/';
        strcpy(&out[1], name);
    } else {
        if (dir_len + 1 + name_len >= out_size) {
            return 0;
        }
        strcpy(out, dir);
        out[dir_len] = '/';
        strcpy(&out[dir_len + 1], name);
    }

    return 1;
}

static int resource_mkdir_parent_dirs(const char *path)
{
    char tmp[RES_UART_PATH_SIZE];
    char *p;

    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            int err;

            *p = '\0';
            err = lfs_mkdir(&lfs, tmp);
            if (err != LFS_ERR_OK && err != LFS_ERR_EXIST) {
                return err;
            }
            *p = '/';
        }
    }

    return LFS_ERR_OK;
}

static void resource_uart_list(void)
{
    lfs_dir_t dir;
    struct lfs_info info;
    int res;

    res = lfs_dir_open(&lfs, &dir, "/");
    if (res < 0) {
        uart1_printf("ERR LIST_OPEN %d\n", res);
        return;
    }

    uart1_printf("OK LIST_BEGIN\n");
    while (1) {
        res = lfs_dir_read(&lfs, &dir, &info);
        if (res < 0) {
            uart1_printf("ERR LIST_READ %d\n", res);
            break;
        }
        if (res == 0) {
            break;
        }

        if (info.type == LFS_TYPE_REG) {
            uart1_printf("FILE %s %lu\n", info.name, (unsigned long)info.size);
        } else if (info.type == LFS_TYPE_DIR) {
            uart1_printf("DIR %s\n", info.name);
        }
    }

    lfs_dir_close(&lfs, &dir);
    uart1_printf("OK LIST_END\n");
}

static void resource_uart_format(char *arg)
{
    int err;

    if (strcmp(arg, "CONFIRM") != 0) {
        uart1_printf("ERR FORMAT_CONFIRM\n");
        return;
    }

    w25q80_close_fs();
    err = lfs_format(&lfs, &cfg_w25q80_fs);
    if (err) {
        uart1_printf("ERR FORMAT %d\n", err);
        return;
    }

    err = lfs_mount(&lfs, &cfg_w25q80_fs);
    if (err) {
        uart1_printf("ERR MOUNT %d\n", err);
        return;
    }

    uart1_printf("OK FORMAT\n");
}

static void resource_uart_delete(char *path)
{
    int err;

    if (!resource_path_valid(path)) {
        uart1_printf("ERR PATH\n");
        return;
    }

    err = lfs_remove(&lfs, path);
    if (err) {
        uart1_printf("ERR DELETE %d\n", err);
        return;
    }

    uart1_printf("OK DELETE\n");
}

#if ENABLE_RESOURCE_RAW_UPLOAD
static void resource_uart_upload(char *path, char *size_str, char *crc_str)
{
    uint32_t size;
    uint32_t expected_crc;
    uint32_t actual_crc = 0xFFFFFFFFUL;
    uint32_t received = 0;
    static uint8_t buf[RES_UART_CHUNK_SIZE];
    lfs_file_t file;
    int err;

    if (!resource_path_valid(path)) {
        uart1_printf("ERR PATH\n");
        return;
    }

    if (!resource_parse_u32_dec(size_str, &size) || size == 0 || size > RES_UART_MAX_FILE_SIZE) {
        uart1_printf("ERR SIZE\n");
        return;
    }

    if (!resource_parse_u32_hex(crc_str, &expected_crc)) {
        uart1_printf("ERR CRC_ARG\n");
        return;
    }

    err = resource_mkdir_parent_dirs(path);
    if (err) {
        uart1_printf("ERR MKDIR %d\n", err);
        return;
    }

    err = lfs_file_open(&lfs, &file, path, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
    if (err) {
        uart1_printf("ERR OPEN %d\n", err);
        return;
    }

    uart1_printf("OK READY\n");

    while (received < size) {
        uint32_t chunk = size - received;
        int got;
        lfs_ssize_t written;

        if (chunk > RES_UART_CHUNK_SIZE) {
            chunk = RES_UART_CHUNK_SIZE;
        }

        got = uart1_read(buf, chunk, RES_UART_READ_TIMEOUT);
        if (got != (int)chunk) {
            lfs_file_close(&lfs, &file);
            lfs_remove(&lfs, path);
            uart1_printf("ERR TIMEOUT %lu\n", (unsigned long)received);
            return;
        }

        written = lfs_file_write(&lfs, &file, buf, chunk);
        if (written != (lfs_ssize_t)chunk) {
            lfs_file_close(&lfs, &file);
            lfs_remove(&lfs, path);
            uart1_printf("ERR WRITE %ld\n", (long)written);
            return;
        }

        actual_crc = resource_crc32_update(actual_crc, buf, chunk);
        received += chunk;
        uart1_printf("OK CHUNK %lu\n", (unsigned long)received);
    }

    err = lfs_file_close(&lfs, &file);
    if (err) {
        lfs_remove(&lfs, path);
        uart1_printf("ERR CLOSE %d\n", err);
        return;
    }

    actual_crc = ~actual_crc;

    if (actual_crc != expected_crc) {
        lfs_remove(&lfs, path);
        uart1_printf("ERR CRC %08lX\n", (unsigned long)actual_crc);
        return;
    }

    uart1_printf("OK DONE %lu %08lX\n", (unsigned long)size, (unsigned long)actual_crc);
}
#endif

#if ENABLE_RESOURCE_YMODEM
typedef struct {
    char dir[RES_UART_PATH_SIZE];
    char path[RES_UART_PATH_SIZE];
    uint32_t size;
    uint32_t written;
    lfs_file_t file;
    uint8_t file_opened;
} resource_ymodem_ctx_t;

static int resource_ymodem_begin(const char *name, uint32_t size, void *ctx)
{
    resource_ymodem_ctx_t *ymodem_ctx = (resource_ymodem_ctx_t*)ctx;
    int err;

    if (size == 0 || size > RES_UART_MAX_FILE_SIZE) {
        return -1;
    }

    if (!resource_join_path(ymodem_ctx->path, sizeof(ymodem_ctx->path), ymodem_ctx->dir, name)) {
        return -1;
    }

    err = resource_mkdir_parent_dirs(ymodem_ctx->path);
    if (err) {
        return err;
    }

    err = lfs_file_open(&lfs, &ymodem_ctx->file, ymodem_ctx->path,
                        LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
    if (err) {
        return err;
    }

    ymodem_ctx->size = size;
    ymodem_ctx->written = 0;
    ymodem_ctx->file_opened = 1;
    return 0;
}

static int resource_ymodem_data(const uint8_t *data, uint32_t len, void *ctx)
{
    resource_ymodem_ctx_t *ymodem_ctx = (resource_ymodem_ctx_t*)ctx;
    lfs_ssize_t written;

    if (!ymodem_ctx->file_opened) {
        return -1;
    }

    if (ymodem_ctx->written + len > ymodem_ctx->size) {
        len = ymodem_ctx->size - ymodem_ctx->written;
    }

    if (len == 0) {
        return 0;
    }

    written = lfs_file_write(&lfs, &ymodem_ctx->file, data, len);
    if (written != (lfs_ssize_t)len) {
        return -1;
    }

    ymodem_ctx->written += len;
    return 0;
}

static int resource_ymodem_end(void *ctx)
{
    resource_ymodem_ctx_t *ymodem_ctx = (resource_ymodem_ctx_t*)ctx;
    int err;

    if (!ymodem_ctx->file_opened) {
        return -1;
    }

    err = lfs_file_close(&lfs, &ymodem_ctx->file);
    ymodem_ctx->file_opened = 0;
    if (err) {
        return err;
    }

    if (ymodem_ctx->written != ymodem_ctx->size) {
        return -1;
    }

    return 0;
}

static void resource_uart_ymodem(char *dir)
{
    resource_ymodem_ctx_t ctx;
    int ret;

    if (!resource_path_valid(dir)) {
        uart1_printf("ERR PATH\n");
        return;
    }

    memset(&ctx, 0, sizeof(ctx));
    strncpy(ctx.dir, dir, sizeof(ctx.dir) - 1);
    ctx.dir[sizeof(ctx.dir) - 1] = '\0';

    uart1_printf("OK YMODEM READY\n");
    ret = ymodem_receive(resource_ymodem_begin,
                         resource_ymodem_data,
                         resource_ymodem_end,
                         &ctx);

    if (ret == YMODEM_OK) {
        uart1_printf("OK YMODEM DONE %s %lu\n", ctx.path, (unsigned long)ctx.written);
    } else {
        if (ctx.file_opened) {
            lfs_file_close(&lfs, &ctx.file);
            ctx.file_opened = 0;
        }
        if (ctx.path[0] != '\0') {
            lfs_remove(&lfs, ctx.path);
        }
        uart1_printf("ERR YMODEM %d\n", ret);
    }
}
#endif

void resource_uart_poll_command(void)
{
    char line[RES_UART_LINE_SIZE];
    char *cmd;
    char *arg1;
    char *arg2;
    char *arg3;

    resource_read_line(line, sizeof(line));

    cmd = strtok(line, " ");
    if (cmd == 0) {
        return;
    }

    if (strcmp(cmd, "PING") == 0) {
        uart1_printf("OK PONG\n");
    } else if (strcmp(cmd, "FS_LIST") == 0) {
        resource_uart_list();
    } else if (strcmp(cmd, "FS_FORMAT") == 0) {
        arg1 = strtok(0, " ");
        resource_uart_format(arg1 ? arg1 : "");
    } else if (strcmp(cmd, "FS_DELETE") == 0) {
        arg1 = strtok(0, " ");
        if (arg1) {
            resource_uart_delete(arg1);
        } else {
            uart1_printf("ERR ARG\n");
        }
#if ENABLE_RESOURCE_YMODEM
    } else if (strcmp(cmd, "FS_YMODEM") == 0) {
        arg1 = strtok(0, " ");
        if (arg1) {
            resource_uart_ymodem(arg1);
        } else {
            uart1_printf("ERR ARG\n");
        }
#endif
#if ENABLE_RESOURCE_RAW_UPLOAD
    } else if (strcmp(cmd, "FS_UPLOAD") == 0) {
        arg1 = strtok(0, " ");
        arg2 = strtok(0, " ");
        arg3 = strtok(0, " ");
        if (arg1 && arg2 && arg3) {
            char path[RES_UART_PATH_SIZE];
            char size_str[16];
            char crc_str[16];

            strncpy(path, arg1, sizeof(path) - 1);
            path[sizeof(path) - 1] = '\0';
            strncpy(size_str, arg2, sizeof(size_str) - 1);
            size_str[sizeof(size_str) - 1] = '\0';
            strncpy(crc_str, arg3, sizeof(crc_str) - 1);
            crc_str[sizeof(crc_str) - 1] = '\0';
            resource_uart_upload(path, size_str, crc_str);
        } else {
            uart1_printf("ERR ARG\n");
        }
#endif
    } else {
        uart1_printf("ERR CMD\n");
    }
}

void resourceUartTask(void *argument)
{
    (void)argument;

    uart1_printf("OK RESOURCE_UART_READY\n");
    for (;;) {
        resource_uart_poll_command();
    }
}
#endif
