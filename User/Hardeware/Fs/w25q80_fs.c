#include "w25q80_fs.h"

#include <string.h>

#include "app_config.h"
#include "cmsis_os2.h"
#include "delay.h"
#include "Gui.h"
#include "Lcd_Driver.h"
#include "MyTask.h"
#include "spi_sel.h"

lfs_t lfs;

#define FONT_CN_INDEX_MAX      64
#define FONT_CN_INDEX_INVALID   0xFFFFFFFFUL

typedef struct
{
    uint8_t utf8[3];
    uint32_t offset;
} font_cn_index_item_t;

static font_cn_index_item_t font_gb12_index[FONT_CN_INDEX_MAX];
static font_cn_index_item_t font_gb16_index[FONT_CN_INDEX_MAX];
static uint16_t font_gb12_index_count = 0;
static uint16_t font_gb16_index_count = 0;
static uint8_t font_gb12_index_ready = 0;
static uint8_t font_gb16_index_ready = 0;

// configuration of the filesystem is provided by this struct
struct lfs_config cfg_w25q80_fs= {
    // block device operations
    .read  = w25q80_read_fs,
    .prog  =w25q80_pro_fs,
    .erase = w25q80_erase_fs,
    .sync  = w25q80_sync_fs,
    .lock=lfs_lock,
    .unlock=lfs_unlock,

    // block device configuration
    .read_size = 256,    //读取16字节
    .prog_size = 256,
    .block_size = 4096,
    .block_count = 256,
    .cache_size = 256,  /* 读写缓冲区大小,设置大一点可以提高读写性能 必须是 read_size和prog_size的整数倍,且是block_size的因子 */
    .lookahead_size = 128, /* 必须是8的倍数 用于缓存分配新的block时的信息,一个bit对应一个block,设置大一点就会搜寻更多的block更有利于均衡 */
    .block_cycles = 500,  /* 设置擦除多少次之后开始进行均衡 设置大读写性能好,但是对均衡平均不利 100-1000 设置-1则不使用磨损均衡*/
};


int w25q80_read_fs(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size)
{
    // osMutexAcquire(mutex_spi1,osWaitForever);
    w25q80_read_data(c->block_size*block+off,(u8*)buffer,size);
    // osMutexRelease(mutex_spi1);
    return LFS_ERR_OK;
}

int w25q80_pro_fs(const struct lfs_config* c, lfs_block_t block, lfs_off_t off, const void* buffer, lfs_size_t size)
{
    w25q80_page_program(c->block_size*block+off,(u8*)buffer,size);
    return w25q80_sync_fs(c);
}

int w25q80_erase_fs(const struct lfs_config* c, lfs_block_t block)
{
    w25q80_sector_erase(block<<12); //擦除4K
    return w25q80_sync_fs(c);
}

int w25q80_sync_fs(const struct lfs_config* c)
{
    if (w25q80_wait_busy()) {
        return LFS_ERR_IO;
    }

    return LFS_ERR_OK;
}
//线程安全

int lfs_lock(const struct lfs_config *c)
{
    if (osKernelGetState() != osKernelRunning)
    {
        return LFS_ERR_OK;

    }
    if (osMutexAcquire(mutex_spi1,osWaitForever)!= osOK)
    {
        LFS_TRACE("Mutex erro");
        return LFS_ERR_IO;
    }
    return LFS_ERR_OK;
}
int lfs_unlock(const struct lfs_config *c)
{
    if (osKernelGetState() != osKernelRunning)
    {
        return LFS_ERR_OK;

    }
    if (osMutexRelease(mutex_spi1)!= osOK)
    {
        return LFS_ERR_IO;
    }
    return LFS_ERR_OK;
}

void w25q80_init_fs()
{
    int err;

#if LFS_FORCE_FORMAT
    /* 强制格式化（开发调试用途） */
    ble_send("Force formatting...\r\n");
    err = lfs_format(&lfs, &cfg_w25q80_fs);
    if (err) {
        ble_send("Format failed: %d\r\n", err);
        while (1);
    }
    ble_send("Format OK\r\n");
#endif

    // 挂载文件系统
    err = lfs_mount(&lfs, &cfg_w25q80_fs);

    if (err) {
        // 挂载失败，进行格式化
        ble_send("Mount failed (%d), formatting...\r\n", err);
        delay_ms(100);

        err = lfs_format(&lfs, &cfg_w25q80_fs);
        if (err) {
            ble_send("Format failed: %d\r\n", err);
            while (1);
        }
        ble_send("Format OK\r\n");

        // 再次尝试挂载
        err = lfs_mount(&lfs, &cfg_w25q80_fs);
        if (err) {
            ble_send("Remount failed after format: %d\r\n", err);
            while (1);
        }
    }
    ble_send("littlefs mounted OK\r\n");
}

static const char *font_cn_path(u8 size)
{
    if (size == 12)
    {
        return "/font/GB12.bin";
    }
    else if (size == 16)
    {
        return "/font/GB16.bin";
    }

    return NULL;
}

static font_cn_index_item_t *font_cn_index_table(u8 size, uint16_t **count, uint8_t **ready)
{
    if (size == 12)
    {
        *count = &font_gb12_index_count;
        *ready = &font_gb12_index_ready;
        return font_gb12_index;
    }
    else if (size == 16)
    {
        *count = &font_gb16_index_count;
        *ready = &font_gb16_index_ready;
        return font_gb16_index;
    }

    *count = NULL;
    *ready = NULL;
    return NULL;
}

static int font_build_cn_index(u8 size)
{
    lfs_file_t file;
    const char *path = font_cn_path(size);
    uint16_t *count = NULL;
    uint8_t *ready = NULL;
    font_cn_index_item_t *table = font_cn_index_table(size, &count, &ready);
    u8 typeFaceNum = (size / 8 + ((size % 8) ? 1 : 0)) * size;
    uint32_t offset = 0;
    int err;

    if ((path == NULL) || (table == NULL))
    {
        return -1;
    }

    if (*ready)
    {
        return 0;
    }

    *count = 0;
    err = lfs_file_open(&lfs, &file, path, LFS_O_RDONLY);
    if (err)
    {
        return -1;
    }

    while (*count < FONT_CN_INDEX_MAX)
    {
        if (lfs_file_read(&lfs, &file, table[*count].utf8, 3) != 3)
        {
            *ready = 1;
            lfs_file_close(&lfs, &file);
            return 0;
        }

        table[*count].offset = offset + 3;
        (*count)++;

        if (lfs_file_seek(&lfs, &file, typeFaceNum, LFS_SEEK_CUR) < 0)
        {
            lfs_file_close(&lfs, &file);
            return -1;
        }

        offset += 3 + typeFaceNum;
    }

    lfs_file_close(&lfs, &file);
    *ready = 0;
    return -1;
}

static uint32_t font_find_offset_CN_cache(const uint8_t *utf8, u8 size)
{
    uint16_t *count = NULL;
    uint8_t *ready = NULL;
    font_cn_index_item_t *table = font_cn_index_table(size, &count, &ready);

    if ((table == NULL) || (font_build_cn_index(size) != 0))
    {
        return FONT_CN_INDEX_INVALID;
    }

    for (uint16_t i = 0; i < *count; i++)
    {
        if (memcmp(table[i].utf8, utf8, 3) == 0)
        {
            return table[i].offset;
        }
    }

    return FONT_CN_INDEX_INVALID;
}

void lfs_list_dir(const char *path)
{
    lfs_dir_t dir;
    struct lfs_info info;

    if (lfs_dir_open(&lfs, &dir, path) < 0)
    {
        ble_send("open dir fail:%s\r\n", path);
        return;
    }

    while (1)
    {
        int res = lfs_dir_read(&lfs, &dir, &info);

        if (res <= 0)
            break;

        // 跳过 . 和 ..
        if (strcmp(info.name, ".") == 0 ||
            strcmp(info.name, "..") == 0)
        {
            continue;
        }

        char fullpath[128];

        if (strcmp(path, "/") == 0)
        {
            xsprintf(fullpath,
                     "/%s",
                     info.name);
        }
        else
        {
            xsprintf(fullpath,
                     "%s/%s",
                     path,
                     info.name);
        }

        if (info.type == LFS_TYPE_REG)
        {
            ble_send("FILE: %s size:%lu\r\n",
                     fullpath,
                     (unsigned long)info.size);
        }
        else if (info.type == LFS_TYPE_DIR)
        {
            ble_send("DIR : %s\r\n", fullpath);

            // 递归进入子目录
            lfs_list_dir(fullpath);
        }
    }

    lfs_dir_close(&lfs, &dir);
}
u16 font_find_offset_CN(u8 *utf8, u8 size) {
    uint32_t offset = font_find_offset_CN_cache(utf8, size);

    if (offset == FONT_CN_INDEX_INVALID)
    {
        return (u16)-1;
    }

    return (u16)offset;
}

int font_read_CN(const char *utf8, uint8_t *buf, u8 size) {

    lfs_file_t file;
    const char *path = font_cn_path(size);
    uint32_t offset = font_find_offset_CN_cache((const uint8_t*)utf8, size);
    u8 typeFaceNum = (size / 8 + ((size % 8) ? 1 : 0)) * size;
    int err;

    if ((path == NULL) || (offset == FONT_CN_INDEX_INVALID))
    {
        return -1;
    }

    err = lfs_file_open(&lfs, &file, path, LFS_O_RDONLY);
    if (err)
    {
        return -1;
    }

    if (lfs_file_seek(&lfs, &file, offset, LFS_SEEK_SET) < 0)
    {
        lfs_file_close(&lfs, &file);
        return -1;
    }

    if (lfs_file_read(&lfs, &file, buf, typeFaceNum) != typeFaceNum)
    {
        lfs_file_close(&lfs, &file);
        return -1;
    }

    lfs_file_close(&lfs, &file);
    return 0;
}

int font_read_Char(const char* utf8, uint8_t* buf, u8 size)
{
    u16 pos=0;
    lfs_file_t file;
    int8_t err=0;
    u8 index=(u8)(*utf8)-' '; //数字index
    if(size==12){
        err=lfs_file_open(&lfs, &file, "/font/ascii_1206.bin", LFS_O_RDONLY);
        pos=index*12;
    }else if(size == 16){
        err=lfs_file_open(&lfs, &file, "/font/ascii_1608.bin", LFS_O_RDONLY);
        pos=index*16;
    }
    if (err)
    {
        return err;
    }
    lfs_file_seek(&lfs, &file,pos, LFS_SEEK_SET);
    lfs_file_read(&lfs, &file, buf,size);

    lfs_file_close(&lfs, &file);
    return 0;

}

int font_read_DIG_Hum(const char* utf8, uint8_t* buf)
{
    u16 pos=0;
    lfs_file_t file;
    int8_t err=0;
    err=lfs_file_open(&lfs, &file, "/font/sz_32.bin", LFS_O_RDONLY);
    if (err)
    {
        return err;
    }
    //0到9
    u8 index=(u8)(*utf8)-'0';
    if (index<10)
    {
        pos=index*128;
    }
    //如果是后面的字符
    if (*utf8 == '.')
    {
        pos=10*128;
    }else if (*utf8 == ':')
    {

        pos=11*128;
    }else if (*utf8 == '%')
    {

        pos=12*128;
    }else if (*utf8 == '℃')
    {

        pos=13*128;
    }else if (*utf8 == '-')
    {

        pos=14*128;
    }
    lfs_file_seek(&lfs, &file,pos, LFS_SEEK_SET);
    lfs_file_read(&lfs, &file, buf,128);

    lfs_file_close(&lfs, &file);
    return 0;
}

