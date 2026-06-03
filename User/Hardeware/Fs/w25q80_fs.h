//
// Created by Meng on 2025/11/24.
//

#ifndef W25Q80_FS_H
#define W25Q80_FS_H
#include "lfs.h"
#include "W25Q80.h"

extern lfs_t lfs;
extern struct lfs_config cfg_w25q80_fs;
/**
 * @brief 从指定块内读数据
 * @param c lfs_config格式参数;
 * @param block 逻辑块编号，从0开始
 * @param off 块内偏移，lfs在调用prog接口时，传入的off值一定能被rprog_size整除
 * @param buffer 写入数据的缓冲区
 * @param size 要读取的字节数，lfs在读取时会确保不会跨块；
 * @retval 0 成功 <0 错误码
 */
int w25q80_read_fs(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size);
/**
 * @brief 从指定块内读数据
 * @param c lfs_config格式参数;
 * @param block 要擦除的逻辑块编号，从0开始
 * @retval 0 成功 <0 错误码
 */
int w25q80_pro_fs(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size);
/**
 * @brief 从指定块内读数据
 * @param c lfs_config格式参数;
 * @param block 要擦除的逻辑块编号，从0开始
 * @return 0 成功 <0 错误码
 */
int w25q80_erase_fs(const struct lfs_config *c, lfs_block_t block);
int w25q80_sync_fs(const struct lfs_config *c);
int lfs_lock(const struct lfs_config *c);
int lfs_unlock(const struct lfs_config *c);
void w25q80_init_fs();
void w25q80_close_fs();

void lfs_list_dir(const char *path);
/**
 * 查找 字符 在文件中的位置
 * @param utf8 汉字
 * @param size 字体大小
 * @return 该字在文件中的位置
 */
u16 font_find_offset_CN(u8 *utf8,u8 size);
/**
 * 读取汉字字符
 * @param utf8 汉字
 * @param buf 该汉字的字模
 * @param size 字体大小
 * @return 0 读成功 -1 读失败
 */
int font_read_CN(const char *utf8, uint8_t *buf,u8 size);

/**
 * 读取汉字字符
 * @param utf8 汉字
 * @param buf 该汉字的字模
 * @param size 字体大小
 * @return 0 读成功 -1 读失败
 */
int font_read_Char(const char *utf8, uint8_t *buf,u8 size);
/**
 * 读取数码管字符
 * @param utf8 汉字
 * @param buf 该汉字的字模
 * @return 0 读成功 -1 读失败
 */
int font_read_DIG_Hum(const char *utf8, uint8_t *buf);



#endif //W25Q80_FS_H

