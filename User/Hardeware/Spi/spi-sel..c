#include "cmsis_os2.h"
#include "MyTask.h"
#include "spi_sel.h"

SPI1_DEV cur_spi1_dev=SPI1_NONE;   //当前的spi1设备
cs_func_t spi1_sel_funcs[SPI1_NONE]={
    SPI_W25Q80_SEL,
    SPI_TFT_SEL,
};
cs_func_t spi1_unsel_funcs[SPI1_NONE]={
    SPI_W25Q80_UNSEL,
    SPI_TFT_UNSEL,
};
void  SPI_W25Q80_SEL(void)
{
    HAL_GPIO_WritePin(GPIOB,SPI_W25Q80_PIN, GPIO_PIN_RESET);
}
void  SPI_W25Q80_UNSEL(void)
{
    HAL_GPIO_WritePin(GPIOB,SPI_W25Q80_PIN, GPIO_PIN_SET);
}
void  SPI_TFT_SEL(void)
{
    HAL_GPIO_WritePin(GPIOB,SPI_TFT_PIN, GPIO_PIN_RESET);
}
void  SPI_TFT_UNSEL(void)
{
    HAL_GPIO_WritePin(GPIOB,SPI_TFT_PIN, GPIO_PIN_SET);
}

void SPI1_Sel_Device(SPI1_DEV dev)
{
    //如果当前有旧设备,先释放
    if (cur_spi1_dev!=SPI1_NONE)
    {
        spi1_unsel_funcs[cur_spi1_dev]();
    }
    //选中当前设备
    spi1_sel_funcs[dev]();
    //更新当前设备
    cur_spi1_dev=dev;
    //注意：锁在 Unsel_Device 中释放，不在这里释放
}

void SPI1_Unsel_Device(SPI1_DEV dev)
{
    //释放掉当前设备（使用cur_spi1_dev而不是参数dev）
    if (cur_spi1_dev!=SPI1_NONE)
    {
        spi1_unsel_funcs[cur_spi1_dev]();  // 修复：使用cur_spi1_dev
    }
    //将当前设备状态更新为无
    cur_spi1_dev=SPI1_NONE;
}
