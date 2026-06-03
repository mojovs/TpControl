//
// Created by Meng on 2024/7/9.
//
#include "adc.h"

#include <stdio.h>
#include "stm32f1xx_ll_adc.h"
#include "cmsis_os2.h"
#include "delay.h"
#include "FreeRTOS.h"
#include "task.h"

ADC_HandleTypeDef adc2_handler = {0};
static int adc_temp={0};
void adc2_init()
{
    adc2_handler.Instance = ADC2;
    adc2_handler.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    adc2_handler.Init.ScanConvMode = ADC_SCAN_ENABLE; //如果开了就是通道依次全扫完一次，回头还会重新扫
    adc2_handler.Init.DiscontinuousConvMode = DISABLE; //规则组通道不连续
    adc2_handler.Init.ContinuousConvMode = DISABLE; //关闭连续转换，进行不连续转换
    adc2_handler.Init.NbrOfConversion = 1;
    adc2_handler.Init.NbrOfDiscConversion = 0; //一次性转换2个通道，总共就两个通道，一次就能结束
    adc2_handler.Init.ExternalTrigConv = ADC_SOFTWARE_START; //软触发
    HAL_StatusTypeDef status=  HAL_ADC_Init(&adc2_handler);
    if (status != HAL_OK)
    {
        return;
    }

    //启动校准
    HAL_ADCEx_Calibration_Start(&adc2_handler);
}

void adc2_config(uint32_t ch, uint32_t rank, uint32_t samp_time)
{
    ADC_ChannelConfTypeDef adc_ch_conf;
    adc_ch_conf.Channel = ch;
    adc_ch_conf.Rank = rank; //序列为1
    adc_ch_conf.SamplingTime = samp_time; //采样时间
    HAL_ADC_ConfigChannel(&adc2_handler, &adc_ch_conf);
}

void humidity_start(void)
{

    adc2_config(ADC_CHANNEL_4, ADC_REGULAR_RANK_1,ADC_SAMPLETIME_239CYCLES_5);
    HAL_ADC_Start(&adc2_handler);
}

u16 humidity_get_value_single(void)
{
    if (HAL_ADC_PollForConversion(&adc2_handler, 2) == HAL_OK)
    {
        return  (u16)HAL_ADC_GetValue(&adc2_handler);
    }

    return -1;
}

void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
    __HAL_RCC_ADC2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio_initer;
    RCC_PeriphCLKInitTypeDef rcc_initer;

    gpio_initer.Pin = GPIO_PIN_4;
    gpio_initer.Mode = GPIO_MODE_ANALOG;
    gpio_initer.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio_initer);

    //配置时钟，为2M
    rcc_initer.PeriphClockSelection = RCC_PERIPHCLK_ADC;    //8M
    rcc_initer.AdcClockSelection = RCC_ADCPCLK2_DIV2;
    HAL_RCCEx_PeriphCLKConfig(&rcc_initer);
}
