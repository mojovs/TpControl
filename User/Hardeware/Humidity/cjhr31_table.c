/**
 * @file    cjhr31_table.c
 * @brief   CJHR-31 湿度传感器 查表实现 (通过 littlefs 读取 W25Q80)
 */

#include "cjhr31_table.h"

#include "lfs.h"
#include "w25q80_fs.h"

/*===========================================================================
 * 小常量数据 — 仍留在内部 Flash (.rodata), 仅 87 字节
 *===========================================================================*/

/** @brief 温度值数组 (索引0=10°C, ..., 索引10=60°C) */
static const uint8_t s_temp[CJHR31_TEMP_COLS] = {
    10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60
};

/** @brief RH% 值数组 (索引0=20%, ..., 索引75=95%) */
static const uint8_t s_rh[CJHR31_RH_ROWS] = {
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
    40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
    50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
    60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
    70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
    80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
    90, 91, 92, 93, 94, 95
};

/*===========================================================================
 * 内部辅助
 *===========================================================================*/

/**
 * @brief 读取文件中一个表项的值
 * @param file  已打开的 littlefs 文件 (打开位置无关 — 内部会 seek)
 * @param rh_idx    RH 行索引 (0~75)
 * @param temp_idx  温度列索引 (0~10)
 * @param[out] val  读取到的 uint32_t 值 (LE)
 * @retval LFS_ERR_OK  成功
 * @retval 其他        错误码
 */
static int table_read_val(lfs_file_t *file, uint16_t rh_idx, uint8_t temp_idx,
                          uint32_t *val)
{
    /* offset = (rh_idx * TEMP_COLS + temp_idx) * sizeof(uint32_t) */
    uint32_t offset = ((uint32_t)rh_idx * CJHR31_TEMP_COLS + temp_idx) * 4;
    uint8_t buf[4];

    int err = lfs_file_seek(&lfs, file, offset, LFS_SEEK_SET);
    if (err < 0) {
        return err;
    }

    lfs_size_t n = lfs_file_read(&lfs, file, buf, 4);
    if (n != 4) {
        return (int)n;  /* 读不足 4 字节视为错误 */
    }

    /* little-endian 组装 */
    *val = (uint32_t)buf[0]
         | ((uint32_t)buf[1] << 8)
         | ((uint32_t)buf[2] << 16)
         | ((uint32_t)buf[3] << 24);
    return LFS_ERR_OK;
}

/*===========================================================================
 * 公开 API
 *===========================================================================*/

int16_t cjhr31_calc_rh(uint32_t r_kohm_x100, int16_t temp_c)
{
    int err;
    lfs_file_t file;

    /* ---- 1. 打开文件 ---- */
    err = lfs_file_open(&lfs, &file, CJHR31_TABLE_PATH, LFS_O_RDONLY);
    if (err) {
        return -2;  /* 文件打开失败 */
    }

    /* ---- 2. 找到温度对应的列 ---- */
    int16_t col_l = 0, col_r = CJHR31_TEMP_COLS - 1;

    if (temp_c <= s_temp[col_l]) {
        col_l = col_r = 0;
    } else if (temp_c >= s_temp[col_r]) {
        col_l = col_r = CJHR31_TEMP_COLS - 1;
    } else {
        while (col_r - col_l > 1) {
            int16_t mid = (col_l + col_r) / 2;
            if (s_temp[mid] <= temp_c) col_l = mid;
            else col_r = mid;
        }
    }

    /* ---- 3. 边界检查 ---- */
    uint32_t r_low, r_high;

    err = table_read_val(&file, 0, (uint8_t)col_l, &r_low);     /* 20%RH (最大阻值) */
    if (err) { lfs_file_close(&lfs, &file); return -2; }

    err = table_read_val(&file, CJHR31_RH_ROWS - 1, (uint8_t)col_l, &r_high); /* 95%RH */
    if (err) { lfs_file_close(&lfs, &file); return -2; }

    if (r_kohm_x100 > r_low || r_kohm_x100 < r_high) {
        lfs_file_close(&lfs, &file);
        return -1;  /* 超出量程 */
    }

    /* ---- 4. 二分查找 RH 区间 (电阻值单调递减) ---- */
    int16_t row_l = 0, row_r = CJHR31_RH_ROWS - 1;

    while (row_r - row_l > 1) {
        int16_t mid = (row_l + row_r) / 2;
        uint32_t r_mid;

        err = table_read_val(&file, (uint16_t)mid, (uint8_t)col_l, &r_mid);
        if (err) { lfs_file_close(&lfs, &file); return -2; }

        if (r_mid >= r_kohm_x100) row_l = mid;  /* 目标值 ≥ 表中值 → 更高 RH 方向 */
        else row_r = mid;
    }

    /* ---- 5. 读取边界值用于插值 ---- */
    uint32_t r_row_l, r_row_r;

    err = table_read_val(&file, (uint16_t)row_l, (uint8_t)col_l, &r_row_l);
    if (err) { lfs_file_close(&lfs, &file); return -2; }

    err = table_read_val(&file, (uint16_t)row_r, (uint8_t)col_l, &r_row_r);
    if (err) { lfs_file_close(&lfs, &file); return -2; }

    lfs_file_close(&lfs, &file);

    /* ---- 6. 线性插值 ---- */
    int16_t rh_l = s_rh[row_l] * 10;   /* x10 */
    int16_t rh_r = s_rh[row_r] * 10;

    if (r_row_l == r_row_r) return rh_l;

    /* rh = rh_l + (rh_r - rh_l) * (r_row_l - r) / (r_row_l - r_row_r) */
    int32_t rh_diff = (int32_t)(rh_r - rh_l)
                    * (int32_t)(r_row_l - r_kohm_x100)
                    / (int32_t)(r_row_l - r_row_r);

    return (int16_t)(rh_l + rh_diff);
}