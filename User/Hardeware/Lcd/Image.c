#include "lfs.h"
#include "w25q80_fs.h"
#include "Image.h"

#include "cmsis_os.h"
#include "MyTask.h"
#include "spi_sel.h"

#if WRITE_IMAGE

#if WRITE_IMAGE_ICON_12X12
#include "icon_12x12.h"
#endif

#if WRITE_IMAGE_ICON_24X24
#include "icon_24x24.h"
#endif

#if WRITE_IMAGE_ICON_24X48
#include "icon_48x24.h"
#endif

void write_images()
{
#if WRITE_IMAGE
   lfs_file_t file;
   // osMutexAcquire(mutex_lfs,osWaitForever);
   lfs_ssize_t total = lfs_fs_size(&lfs); // 已用的 block 数
   ble_send("Used blocks: %d \n", total);
   SPI1_Sel_Device(SPI1_FLASH);
   // 挂载
   u8 err=lfs_mkdir(&lfs,"/images");
   if (err != LFS_ERR_OK)
   {
      ble_send("mkdir err\n");
   }
#if WRITE_IMAGE_ICON_12X12
   write_icon_12x12();
#endif
#if WRITE_IMAGE_ICON_24X24
   write_icon_24x24();
#endif
#if WRITE_IMAGE_ICON_48x24
   write_icon_24x48();
#endif
   SPI1_Unsel_Device(SPI1_FLASH);
#endif
}

void write_img(const char *path,const char *img_buf,uint32_t size)
{
   lfs_file_t file;
   int16_t err;
   err =lfs_file_open(&lfs,&file,path,LFS_O_WRONLY|LFS_O_CREAT);
   if (err != LFS_ERR_OK)
   {
      ble_send("file open err\n");

   }
   lfs_file_write(&lfs,&file,img_buf,size);
   lfs_file_close(&lfs,&file);
}

#if WRITE_IMAGE_ICON_12X12
void write_icon_12x12(void)
{
   write_img("/images/add_12x12.bin", gImage_add_12x12, sizeof(gImage_add_12x12));
   write_img("/images/clock_12x12.bin", gImage_clock_12x12, sizeof(gImage_clock_12x12));
   write_img("/images/fan_12x12.bin", gImage_fan_12x12, sizeof(gImage_fan_12x12));
   write_img("/images/hot_12x12.bin", gImage_hot_12x12, sizeof(gImage_hot_12x12));
   write_img("/images/hum_12x12.bin", gImage_hum_12x12, sizeof(gImage_hum_12x12));
   write_img("/images/leftArrow_12x12.bin", gImage_leftArrow_12x12, sizeof(gImage_leftArrow_12x12));
   write_img("/images/reduce_12x12.bin", gImage_reduce_12x12, sizeof(gImage_reduce_12x12));
   write_img("/images/rightArrow_12x12.bin", gImage_rightArrow_12x12, sizeof(gImage_rightArrow_12x12));
   write_img("/images/set_12x12.bin", gImage_set_12x12, sizeof(gImage_set_12x12));
}
#endif

#if WRITE_IMAGE_ICON_24X24
void write_icon_24x24(void)
{
   write_img("/images/bean_24x24.bin", gImage_bean_24x24, sizeof(gImage_bean_24x24));
   write_img("/images/bread_24x24.bin", gImage_bread_24x24, sizeof(gImage_bread_24x24));
   write_img("/images/chicken_24x24.bin", gImage_chicken_24x24, sizeof(gImage_chicken_24x24));
   write_img("/images/egg_24x24.bin", gImage_egg_24x24, sizeof(gImage_egg_24x24));
   write_img("/images/fan_24x24.bin", gImage_fan_24x24, sizeof(gImage_fan_24x24));
   write_img("/images/heat_24x24.bin", gImage_heat_24x24, sizeof(gImage_heat_24x24));
   write_img("/images/hum_24x24.bin", gImage_hum_24x24, sizeof(gImage_hum_24x24));
   write_img("/images/printer_24x24.bin", gImage_printer_24x24, sizeof(gImage_printer_24x24));
   write_img("/images/start_24x24.bin", gImage_start_24x24, sizeof(gImage_start_24x24));
   write_img("/images/temp_24x24.bin", gImage_temp_24x24, sizeof(gImage_temp_24x24));
   write_img("/images/temp_curve_24x24.bin", gImage_temp_curve_24x24, sizeof(gImage_temp_curve_24x24));
}
#endif

#if WRITE_IMAGE_ICON_48x24
void write_icon_48x24(void)
{
   write_img("/images/mode_button_48x24.bin", gImage_mode_button_48x24, sizeof(gImage_mode_button_48x24));
   write_img("/images/start_button_48x24.bin", gImage_start_button_48x24, sizeof(gImage_start_button_48x24));
   write_img("/images/stop_button_48x24.bin", gImage_stop_button_48x24, sizeof(gImage_stop_button_48x24));
}
#endif

#endif
