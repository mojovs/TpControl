//
// Created by meng on 2024/6/7.
//

#include "tim.h"

#include "adc.h"
#include "led.h"
#include "main.h"
#include"stdio.h"
#include "stm32f1xx_ll_gpio.h"
#include "GUI.h"
#include "MyTask.h"
#include "queue.h"
//捕获状态


//[1],0没有成功的捕获;1,成功捕获到一次.
//[0] ,0没有捕获到上升沿,捕获到上升沿
u8 TIM2CH1_CAPTURE_STA = 0; //输入捕获状态

extern osMessageQueueId_t detect_queue;;
extern osMessageQueueId_t led_queue;

osMutexId mutex;

//u32 TIM2CH1_CAPTURE_VAL; //输入捕获值(TIM2/TIM5是32位)
TIM_HandleTypeDef tim1_handler = {0};
TIM_OC_InitTypeDef tim1_oc_initer = {0};

TIM_HandleTypeDef tim2_handler = {0};
TIM_IC_InitTypeDef tim2_ic_initer = {0};

TIM_HandleTypeDef tim3_handler = {0};
TIM_OC_InitTypeDef tim3_oc_initer = {0};

void tim1_init()
{
    //一般是用，72/72=1MHz 1MHz/100=10Khz，T=100us
    //arr那么设为10000,即为1s
    tim1_handler.Instance = TIM1;
    tim1_handler.Init.Prescaler = 72 - 1; //
    tim1_handler.Init.Period = 100 - 1;
    tim1_handler.Init.CounterMode = TIM_COUNTERMODE_UP; //向上计数
    tim1_handler.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    tim1_handler.Init.RepetitionCounter = 0;


    HAL_TIM_Base_Init(&tim1_handler);

    HAL_NVIC_SetPriority(TIM1_UP_IRQn, 5, 0); //设置中断优先级，抢占优先级3，子优先级0
    HAL_NVIC_EnableIRQ(TIM1_UP_IRQn); //开启ITM3中断通道
}

u8 tim1_step = 0;

void TIM1_UP_IRQHandler(void)
{
    if (__HAL_TIM_GET_FLAG(&tim1_handler, TIM_FLAG_UPDATE) != RESET)
    {
        if (__HAL_TIM_GET_IT_SOURCE(&tim1_handler, TIM_IT_UPDATE) != RESET)
        {
            //__HAL_TIM_CLEAR_IT(&tim1_handler, TIM_IT_UPDATE); //用这个只清除了中断pending，没有清除标志位
            __HAL_TIM_CLEAR_FLAG(&tim1_handler, TIM_IT_UPDATE);

            //print_string_gui(0,12,"count:%d",count_tim1);
            //
            if (tim1_step == 0)
            {
                //上极拉高，下极拉低

                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_SET);
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
            }
            else if (tim1_step == 3)
            //开始adc采样
            {
                osThreadFlagsSet(humidity_ac_task_hdl, 0x1); //发送adc采样命令
            }
            //5*0.1ms后，上极拉低，下极拉高
            else if (tim1_step == 5)
            {
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_SET);
            }
            else
            {

            }
            if (++tim1_step == 10)
            {
                tim1_step = 0;
            }
        }
    }
}

//人体检测BISS传感器
void tim2_init()
{
    tim2_handler.Instance = TIM2;
    //一般是用，8M/80=100 KHz f=10us，100arr=1ms  1kHz
    //arr那么设为10000,即为1s
    tim2_handler.Init.Prescaler = 80 - 1; //
    tim2_handler.Init.Period = 100 - 1;
    tim2_handler.Init.CounterMode = TIM_COUNTERMODE_UP; //向上计数
    tim2_handler.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    tim2_ic_initer.ICPolarity = TIM_ICPOLARITY_RISING; //上升沿捕获
    tim2_ic_initer.ICSelection = TIM_ICSELECTION_DIRECTTI; //映射到TI1上
    tim2_ic_initer.ICPrescaler = TIM_ICPSC_DIV1; //配置输入分频，不分频
    tim2_ic_initer.ICFilter = 0; //配置输入滤波器，不滤波
    HAL_TIM_IC_Init(&tim2_handler); //初始化输入捕获时基参数
    HAL_TIM_IC_ConfigChannel(&tim2_handler, &tim2_ic_initer, TIM_CHANNEL_1); //配置TIM2通道1

    //PA0开始捕获
    HAL_TIM_IC_Start_IT(&tim2_handler, TIM_CHANNEL_1); //开启TIM2的捕获通道1，并且开启捕获中断

    //使能中断
    __HAL_TIM_ENABLE_IT(&tim2_handler, TIM_IT_UPDATE); //使能更新中断
}

void tim3_init()
{
    tim3_handler.Instance = TIM3;
    //72M/72=1M   1M/1000=1KHz
    tim3_handler.Init.Prescaler = 72 - 1; //
    tim3_handler.Init.Period = 1000 - 1;
    tim3_handler.Init.CounterMode = TIM_COUNTERMODE_UP; //向上计数
    tim3_handler.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;

    tim3_oc_initer.OCMode = TIM_OCMODE_PWM1; //
    tim3_oc_initer.Pulse = 100;
    tim3_oc_initer.OCPolarity = TIM_OCPOLARITY_HIGH;

    HAL_TIM_PWM_Init(&tim3_handler);
    HAL_TIM_PWM_ConfigChannel(&tim3_handler, &tim3_oc_initer, TIM_CHANNEL_1);
    HAL_TIM_PWM_ConfigChannel(&tim3_handler, &tim3_oc_initer, TIM_CHANNEL_4);

    HAL_TIM_PWM_Start(&tim3_handler,TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&tim3_handler,TIM_CHANNEL_4);
}


//给灯 和指示灯提供PWM
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef* htim)
{
    //定时器2，检测信号输入
    GPIO_InitTypeDef gpio_initer;
    __HAL_AFIO_REMAP_TIM3_PARTIAL();

    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_TIM3_CLK_ENABLE(); //使能TIM2时钟
    __HAL_RCC_GPIOB_CLK_ENABLE(); //使能TIM2时钟

    gpio_initer.Pin =  GPIO_PIN_4;
    gpio_initer.Speed = GPIO_SPEED_HIGH;
    gpio_initer.Pull = GPIO_NOPULL; //开始为拉高
    gpio_initer.Mode = GPIO_MODE_AF_PP;
    HAL_GPIO_Init(GPIOB, &gpio_initer);

    // //设置PB4
    // gpio_initer.Pin = GPIO_PIN_4;
    // gpio_initer.Speed = GPIO_SPEED_HIGH;
    // gpio_initer.Pull = GPIO_PULLUP;
    // gpio_initer.Mode = GPIO_MODE_OUTPUT_PP;
    // HAL_GPIO_Init(GPIOB, &gpio_initer);

    HAL_NVIC_SetPriority(TIM3_IRQn, 4, 0); //设置中断优先级，抢占优先级3，子优先级0
    HAL_NVIC_EnableIRQ(TIM3_IRQn); //开启ITM3中断通道
}
