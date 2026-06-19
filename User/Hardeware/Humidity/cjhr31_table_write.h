/**
 * @file    cjhr31_table_write.h
 * @brief   CJHR-31 查表数据 — 首次烧录写入 W25Q80 (littlefs)
 *
 * 正常固件不包含此模块（FLASH_WRITE=0 时不编译）。
 * 用 cmake -DFLASH_WRITE=ON -DWRITE_CJHR31=ON .. 构建烧写版本。
 *
 * 写入路径: /sys/rh_table.bin
 * 数据格式: 纯二进制 uint32_t LE, row-major
 *           76 行 × 11 列 × 4 字节 = 3,344 字节
 */

#ifndef CJHR31_TABLE_WRITE_H
#define CJHR31_TABLE_WRITE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if FLASH_WRITE
/**
 * @brief 将 CJHR-31 查表数据写入 littlefs (/sys/rh_table.bin)
 *
 * 仅用于首次烧录，正常固件不包含此函数。
 *
 * @return 0 成功, -1 失败
 */
int cjhr31_table_write_to_lfs(void);
#endif /* FLASH_WRITE */

#ifdef __cplusplus
}
#endif

#endif /* CJHR31_TABLE_WRITE_H */