/**
 * @file    cjhr31_table.h
 * @brief   CJHR-31 湿度传感器 电阻-湿度-温度 查表接口
 *
 * 数据存储在 W25Q80 (littlefs) 中: /sys/rh_table.bin
 * 共 76 行 × 11 列 = 836 个 uint32_t 值, 3,344 字节
 *
 * 测试条件: LCR 1KHz, 1VAC, ±3%RH
 * 数据来源: CJHR31湿度传感器规格书.pdf
 *
 * 使用方法:
 *   1. 测得的电阻值 R_meas (kΩ)
 *   2. 获取当前温度 T (°C)
 *   3. 调用 cjhr31_calc_rh(R_meas_x100, T) 获取 RH%
 */

#ifndef CJHR31_TABLE_H
#define CJHR31_TABLE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 温度列数 */
#define CJHR31_TEMP_COLS  11

/** @brief 湿度行数 (20% ~ 95%RH) */
#define CJHR31_RH_ROWS    76

/** @brief littlefs 中表文件的路径 */
#define CJHR31_TABLE_PATH "/sys/rh_table.bin"

/**
 * @brief 由电阻和温度查 RH% (线性插值)
 * @param r_kohm_x100  实测电阻值, 单位 kΩ × 100 (如 38.04kΩ → 3804)
 * @param temp_c       当前温度, 单位 °C
 * @return RH% 值 (带 1 位小数, x10 返回, 如 65.3% → 653)
 *         返回 -1 表示超出查表范围, -2 表示文件读取失败
 */
int16_t cjhr31_calc_rh(uint32_t r_kohm_x100, int16_t temp_c);

#ifdef __cplusplus
}
#endif

#endif /* CJHR31_TABLE_H */