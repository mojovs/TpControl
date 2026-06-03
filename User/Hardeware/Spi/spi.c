//
// Created by Meng on 2024/7/14.
//

#include "spi.h"

SPI_HandleTypeDef spi_handler_1={0};
void spi1_init()
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();

    /*
     * PA5:SPI_SCK
     * PA7:SPI1_MOSI Master out slave in
     */
    //spi初始化
    spi_handler_1.Instance=SPI1;
    spi_handler_1.Init.Mode =SPI_MODE_MASTER;
    spi_handler_1.Init.Direction=SPI_DIRECTION_2LINES;
    spi_handler_1.Init.DataSize=SPI_DATASIZE_8BIT;
    //空闲低电平，上升沿触筏
    spi_handler_1.Init.CLKPolarity=SPI_POLARITY_HIGH;
    spi_handler_1.Init.CLKPhase=SPI_PHASE_2EDGE;
    spi_handler_1.Init.NSS=SPI_NSS_SOFT; //软件片选
    //72M/4=18M
    spi_handler_1.Init.BaudRatePrescaler=SPI_BAUDRATEPRESCALER_4;
    //msb,lsb，msb为先发送高位
    spi_handler_1.Init.FirstBit=SPI_FIRSTBIT_MSB;
    spi_handler_1.Init.TIMode=SPI_TIMODE_DISABLE;
    spi_handler_1.Init.CRCCalculation=SPI_CRCCALCULATION_DISABLE;   //关闭硬件CRC计算
    spi_handler_1.Init.CRCPolynomial=7; //CRC值计算多项式

    HAL_SPI_Init(&spi_handler_1);
    spi1_set_speed(2);
    __HAL_SPI_ENABLE(&spi_handler_1);
}

void spi1_set_speed(u8 speed)
{
    assert_param(IS_SPI_BAUDRATE_PRESCALER(speed));
    //关闭SPI
    __HAL_SPI_DISABLE(&spi_handler_1);
    spi_handler_1.Instance->CR1&=0xFFC7;
    spi_handler_1.Instance->CR1|=speed<<3;
    __HAL_SPI_ENABLE(&spi_handler_1);
}

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();
    //初始化gpio
    GPIO_InitTypeDef gpio_initer;
    gpio_initer.Pin=GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
    gpio_initer.Mode=GPIO_MODE_AF_PP;
    gpio_initer.Pull=GPIO_NOPULL;
    gpio_initer.Speed=GPIO_SPEED_HIGH;
    HAL_GPIO_Init(GPIOA,&gpio_initer);

}
