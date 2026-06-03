//
// Created by Meng on 2025/11/21.
//

#ifndef KEY_C_H
#define KEY_C_H
#include "key.h"
#include "delay.h"

/* KEY_A	按键	PB8
* KEY_B	按键	PB3
* EC_L	旋转编码器左旋	PB14
* EC_R	旋转编码器左旋	PB15
* EC_DOWN	旋转编码器按下	PB9
*/


void key_init()
{
    GPIO_InitTypeDef gpioInitTypeDef;
    __HAL_RCC_GPIOB_CLK_ENABLE();

    //按键 A B
    gpioInitTypeDef.Mode = GPIO_MODE_IT_RISING;
    gpioInitTypeDef.Pull = GPIO_PULLDOWN;
    gpioInitTypeDef.Speed = GPIO_SPEED_HIGH;
    gpioInitTypeDef.Pin = GPIO_PIN_3 | GPIO_PIN_8;
    HAL_GPIO_Init(GPIOB, &gpioInitTypeDef);

    //旋转编码器按下
    gpioInitTypeDef.Pin=GPIO_PIN_9;
    gpioInitTypeDef.Mode = GPIO_MODE_IT_FALLING;
    gpioInitTypeDef.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &gpioInitTypeDef);

    //旋转编码器A相
    gpioInitTypeDef.Pin = GPIO_PIN_14;
    gpioInitTypeDef.Mode = GPIO_MODE_IT_FALLING;
    HAL_GPIO_Init(GPIOB, &gpioInitTypeDef);
    //旋转编码器A相
    gpioInitTypeDef.Pin = GPIO_PIN_15;
    gpioInitTypeDef.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(GPIOB, &gpioInitTypeDef);

    HAL_NVIC_SetPriority(EXTI3_IRQn, 4, 0);
    HAL_NVIC_EnableIRQ(EXTI3_IRQn);


    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 4, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 4, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

void ec11_spin(void)
{
    //检测B相，为高，那么则顺时针
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15) ==1)
    {

    }else
    {

    }

}
void ec11_add_redcut(u8* val, u16 max_min, u8 mode)
{
    if (mode == 1) //加
    {
        if ((*val) < max_min)
            (*val)++;
    }
    else  if(mode ==0)//减
    {
        if ((*val) > max_min)
            (*val)--;
    }
}



void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    switch (GPIO_Pin)
    {
        //EC11 A相的下降沿的时候
        case GPIO_PIN_14:

        break;
        // 按键A
        case GPIO_PIN_8:
        break;
        // 按键B
        case GPIO_PIN_3:
        break;

        // EC11 按下
        case GPIO_PIN_9:
        break;
    }
}

void EXTI15_10_IRQHandler()
{
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_14))
    {
        // 清除中断标志位
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_14);
        HAL_GPIO_EXTI_Callback(GPIO_PIN_14);
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_14);
    }
    else if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_15))
    {
        // 清除中断标志位
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_15);
        HAL_GPIO_EXTI_Callback(GPIO_PIN_15);
        //清中断，防抖动
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_15);
    }
}

void EXTI9_5_IRQHandler()
{

//按键A
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_8))
    {
        // 清除中断标志位
            __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_8);
            HAL_GPIO_EXTI_Callback(GPIO_PIN_8);
            __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_8);
    }
//EC_DOWN
    else if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_9))
    {
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_9);
        HAL_GPIO_EXTI_Callback(GPIO_PIN_9);
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_9);
    }
}
//按键B
void EXTI3_IRQHandler()
{
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_3))
    {
        // 清除中断标志位
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_3);
        HAL_GPIO_EXTI_Callback(GPIO_PIN_3);
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_3);
    }
}

#endif //KEY_C_H

