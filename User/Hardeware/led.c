//
// Created by meng on 2024/5/8.
//

#include "led.h"

#include "delay.h"
#include "FreeRTOS.h"

#include "task.h"

void light_gradient(u16 from, u16 to, u16 sec) {
    u16 val_comp=from;  //比较值
    u16 delta=0;    //从渐变开始到结束的比较值差值
    //亮度增大
    if (to > from) {
        delta=to-from;
        u32 delay_time=sec*1000/(delta); //延迟多少ms
        for(int i=0;i<delta;i++){
            val_comp++;
            light_set_compare(val_comp);
            osDelay(delay_time);    //延迟多少s
        }
    } else {
        delta=from-to;
        u32 delay_time=sec*1000/(delta); //延迟多少ms
        for(int i=0;i<delta;i++){
            val_comp--;
            light_set_compare(val_comp);
            osDelay(delay_time);
        }
    }

}

void light_breath(u8 level, u8 speed, u16 cnt) {
    u8 dir=1;   //1递增模式
    u16 led0_pwm_val=0;
    u16 cur_cnt=cnt*2;
    //调整占空比
    while (cur_cnt) {
        osDelay(speed);
        if (dir) led0_pwm_val++;
        else led0_pwm_val--;
        if (led0_pwm_val >tim1_handler.Init.Period * level / 20)
        {
            dir = 0; //到达设置的arr值，开始递减
            cur_cnt--;
        }
        if (led0_pwm_val == 0){ dir = 1; cur_cnt--;};
        light_set_compare(led0_pwm_val);
    }
        light_set_compare(0);

}



void led_init()
{
    GPIO_InitTypeDef gpio_initer;
    __HAL_RCC_GPIOB_CLK_ENABLE();
    gpio_initer.Mode=GPIO_MODE_OUTPUT_PP;
    gpio_initer.Pin=GPIO_PIN_1;
    gpio_initer.Speed=GPIO_SPEED_MEDIUM;
    gpio_initer.Pull=GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB,&gpio_initer);
}
//PWM模式
void led_set_compare(u32 comp)
{

    TIM3->CCR4=comp;   //设置tim1ch1通道
}
void led_breath(u8 level, u8 speed, u16 cnt) {
    u8 dir=1;   //1递增模式
    u16 led0_pwm_val=0;
    u16 cur_cnt=cnt*2;
    //调整占空比
    while (cur_cnt) {
        delay_ms(speed);
        if (dir) led0_pwm_val++;
        else led0_pwm_val--;
        if (led0_pwm_val >tim3_handler.Init.Period * level / 20)
        {
            dir = 0; //到达设置的arr值，开始递减
            cur_cnt--;
        }
        if (led0_pwm_val == 0){ dir = 1; cur_cnt--;};
        led_set_compare(led0_pwm_val);
    }
    led_set_compare(0);

}
void led_on()
{
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_RESET);
}

void led_off()
{
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_SET);
}

void led_shine(u16 times, u16 delay)
{
    for(int i=0;i<times;i++)
    {
        delay_ms(delay);
        led_on();
        delay_ms(delay);
        led_off();

    }
    led_off();
}

void light_very_high(u32 sec)
{
    light_set_compare(tim1_handler.Init.Period *7/8);
    osDelay(sec*1000);
    light_set_compare(0);
}

void light_high(u32 sec)
{
    light_set_compare(tim1_handler.Init.Period *5/8);
    osDelay(sec*1000);
    light_set_compare(0);
}

void light_medium(u32 sec)
{
    light_set_compare(tim1_handler.Init.Period *2/8);
    osDelay(sec*1000);
    light_set_compare(0);
}

void light_low(u32 sec)
{

    light_set_compare(tim1_handler.Init.Period *1/8);
    osDelay(sec*1000);
    light_set_compare(0);
}


void light_set_compare(u32 comp)
{

    taskENTER_CRITICAL();
    TIM1->CCR1=comp;   //设置tim1ch1通道
    taskEXIT_CRITICAL();
}

