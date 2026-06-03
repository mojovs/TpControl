#include "buzz.h"
#include "delay.h"

void buzz_init(void)
{
    GPIO_InitTypeDef gpio_initer;
    __HAL_RCC_GPIOA_CLK_ENABLE();
    gpio_initer.Mode=GPIO_MODE_OUTPUT_PP;
    gpio_initer.Pin=GPIO_PIN_15;
    gpio_initer.Speed=GPIO_SPEED_MEDIUM;
    gpio_initer.Pull=GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOA,&gpio_initer);
    buzz_off();
}

void buzz_play(u32 time)
{
    buzz_on();
    delay_ms(time);
    buzz_off();


}

void buzz_off(void)
{
    HAL_GPIO_WritePin(GPIOA,GPIO_PIN_15,GPIO_PIN_RESET);
}

void buzz_on(void)
{
    HAL_GPIO_WritePin(GPIOA,GPIO_PIN_15,GPIO_PIN_SET);
}
