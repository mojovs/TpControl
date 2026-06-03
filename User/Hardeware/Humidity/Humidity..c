#include "cmsis_os2.h"
#include "delay.h"
#include "Humidity.h"
void humidity_init(void)
{
    GPIO_InitTypeDef gpio_initer;
    __HAL_RCC_GPIOA_CLK_ENABLE();
    gpio_initer.Pin = GPIO_PIN_11 | GPIO_PIN_12;
    gpio_initer.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_initer.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio_initer.Pull = GPIO_PULLDOWN;   //默认都为低电平，防止伤害湿敏电阻
    HAL_GPIO_Init(GPIOA, &gpio_initer);
    //设置默认
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);
}

