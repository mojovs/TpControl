//
// Created by Meng on 2025/12/3.
//

#ifndef TPCONTROL_SPI_SEL_H
#define TPCONTROL_SPI_SEL_H
#include "sys.h"


typedef enum _SPI1_DEV
{
    SPI1_FLASH,
    SPI1_TFT,
    SPI1_NONE
}SPI1_DEV;
typedef void (*cs_func_t)(void);    //片选函数
extern SPI1_DEV cur_spi1_dev;   //当前的spi1设备
#define SPI_W25Q80_PIN GPIO_PIN_0

void  SPI_W25Q80_SEL(void);
void  SPI_W25Q80_UNSEL(void);
#define SPI_TFT_PIN GPIO_PIN_7
void  SPI_TFT_SEL(void);
void  SPI_TFT_UNSEL(void);
/**
 * 片选设备
 * @param dev 枚举例，选择哪个设备
 */
void SPI1_Sel_Device(SPI1_DEV dev);

/**
 * 取消片选
 * @param dev 取消片选设备
 */
void SPI1_Unsel_Device(SPI1_DEV dev);
/**
 * 取消片选所有设备
 * @param dev 取消片选设备
 */
void SPI1_Unsel__All_Device(SPI1_DEV dev);

#endif //TPCONTROL_SPI_SEL_H

