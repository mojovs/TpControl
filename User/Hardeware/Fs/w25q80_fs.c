#include"w25q80_fs.h"

#include "cmsis_os2.h"
#include "delay.h"
#include "Gui.h"
#include "Lcd_Driver.h"
#include "MyTask.h"
#include "spi_sel.h"

lfs_t lfs;
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
    .read_size = 64,    //读取16字节
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
    if (osMutexAcquire(mutex_spi1,osWaitForever)!= osOK)
    {
        LFS_TRACE("Mutex erro");
        return LFS_ERR_IO;
    }
    return LFS_ERR_OK;
}
int lfs_unlock(const struct lfs_config *c)
{
    if (osMutexRelease(mutex_spi1)!= osOK)
    {
        return LFS_ERR_IO;
    }
    return LFS_ERR_OK;
}

void w25q80_init_fs()
{
    //挂载
    //选中flash

    int err=lfs_mount(&lfs,&cfg_w25q80_fs);
    if (err) {
        // 挂载失败，进行格式化
        //Gui_ShowString(48,0,"Mount failed ,format",RED,WHITE,12,0);
        delay_ms(1000);
        lfs_format(&lfs, &cfg_w25q80_fs);
        // 再次尝试挂载
        err = lfs_mount(&lfs, &cfg_w25q80_fs);
        if (err) {
            // 格式化后挂载仍然失败，处理错误
            while(1);
        }
    }
}

void w25q80_close_fs()
{
    lfs_unmount(&lfs);  //解除挂载
}
void mount_lfs(){
    lfs_mount(&lfs,&cfg_w25q80_fs);
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
            snprintf(fullpath,
                     sizeof(fullpath),
                     "/%s",
                     info.name);
        }
        else
        {
            snprintf(fullpath,
                     sizeof(fullpath),
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

    lfs_file_t file;
    u8 err=0;
    u8 typeFaceNum = (size / 8 + ((size % 8) ? 1 : 0)) * size; //

    if(size==12){
        err=lfs_file_open(&lfs, &file, "/font/GB12.bin", LFS_O_RDONLY);
    }else if(size == 16){
        err=lfs_file_open(&lfs, &file, "/font/GB16.bin", LFS_O_RDONLY);
    }
    if (err)
    {
        return err;
    }

    uint8_t buf[3];

    for(int i=0; ;i++){
        //读3个
        if(lfs_file_read(&lfs,&file, buf, 3) != 3)
            break;

        //对比下和输入参数的区别
        if(memcmp(buf, utf8, 3)==0){
            lfs_file_close(&lfs,&file);
                return (typeFaceNum+3)*i+3; // 指向点阵开始位置
        }

        //再进24个，跳到下一个字符组
        lfs_file_seek(&lfs,&file, typeFaceNum, LFS_SEEK_CUR);
    }

    lfs_file_close(&lfs,&file);
    return -1;

}

int font_read_CN(const char *utf8, uint8_t *buf, u8 size) {

    int offset = font_find_offset_CN(utf8,size);
    if(offset < 0){
        return -1;
    }
    u8 typeFaceNum = (size / 8 + ((size % 8) ? 1 : 0)) * size; //
    lfs_file_t file;
    char  err=0;
    if(size==12){
        err=lfs_file_open(&lfs, &file, "/font/GB12.bin", LFS_O_RDONLY);
    }else if(size == 16){
        err=lfs_file_open(&lfs, &file, "/font/GB16.bin", LFS_O_RDONLY);
    }
    if (err)
    {
        return -1;
    }

    lfs_file_seek(&lfs, &file, offset, LFS_SEEK_SET);
    lfs_file_read(&lfs, &file, buf,typeFaceNum );

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

