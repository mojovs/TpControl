#include "Temperature.h"

u8 temperature_init()
{
    GPIO_InitTypeDef gpioInitTypeDef;
    __HAL_RCC_GPIOA_CLK_ENABLE();
    gpioInitTypeDef.Mode = GPIO_MODE_OUTPUT_OD; //上拉 开漏输出
    gpioInitTypeDef.Pull = GPIO_PULLUP;
    gpioInitTypeDef.Speed = GPIO_SPEED_HIGH;
    gpioInitTypeDef.Pin = GPIO_PIN_0;
    HAL_GPIO_Init(GPIOA, &gpioInitTypeDef);
    ds18b20_reset();
    return ds18b20_check();
}
void ds18b20_reset(void)
{
    DS18B20_DQ_OUT(0); /* 拉低 DQ,复位 */
    delay_us(750); /* 拉低 750us */
    DS18B20_DQ_OUT(1); /* DQ=1, 释放复位 */
    delay_us(15); /* 延迟 15US */
}

u8 ds18b20_check(void)
{
    uint8_t retry = 0;
    uint8_t rval = 0;
    while (DS18B20_DQ_IN && retry < 200) /* 等待 DQ 变低, 等待 200us */
    {
        retry++;
        delay_us(1);
    }
    if (retry >= 200)
    {
        rval = 1; //返回异常
    }
    else
    {
        retry = 0;
        while (!DS18B20_DQ_IN && retry < 240) /* 等待 DQ 变高, 等待 240us */
        {
            retry++;
            delay_us(1);
        }
        if (retry >= 240) rval = 1;
    }
    return rval;
}
void ds18b20_write_byte(uint8_t data)
{
    uint8_t j;
    for (j = 1; j <= 8; j++)
    {
        if (data & 0x01)
        {
            DS18B20_DQ_OUT(0); /* Write 1 */
            delay_us(2);
            DS18B20_DQ_OUT(1);
            delay_us(60);
        }
        else
        {
            DS18B20_DQ_OUT(0); /* Write 0 */
            delay_us(60);
            DS18B20_DQ_OUT(1);
            delay_us(2);
        }
        data >>= 1; /* 右移,获取高一位数据 */
    }
}
uint8_t ds18b20_read_bit(void)
{
    uint8_t data = 0;
    DS18B20_DQ_OUT(0);
    delay_us(2);
    DS18B20_DQ_OUT(1);
    delay_us(12);
    if (DS18B20_DQ_IN)
    {
        data = 1;
    }
    delay_us(50);
    return data;
}
/**
* @brief 从 DS18B20 读取一个字节
* @param 无
* @retval 读到的数据
*/
uint8_t ds18b20_read_byte(void)
{
    uint8_t i, b, data = 0;
    for (i = 0; i < 8; i++)
    {
        b = ds18b20_read_bit(); /* DS18B20 先输出低位数据 ,高位数据后输出 */
        data |= b << i; /* 填充 data 的每一位 */
    }
    return data;
}
void ds18b20_start(void)
{
    ds18b20_reset();
    ds18b20_check();
    ds18b20_write_byte(0xcc); /* skip rom */
    ds18b20_write_byte(0x44); /* convert */
}
/**
* @brief 从 ds18b20 得到温度值(精度： 0.1C)
* @param 无
* @retval 温度值 （-550~1250）
* @note 返回的温度值放大了 10 倍.
* 实际使用的时候,要除以 10 才是实际温度.
*/
short ds18b20_get_temperature(void)
{
    uint8_t TL, TH;
    short temp;
    ds18b20_start(); /* ds1820 start convert */
    ds18b20_reset();
    ds18b20_check();
    ds18b20_write_byte(0xcc); /* skip rom */
    ds18b20_write_byte(0xbe); /* convert */
    TL = ds18b20_read_byte(); /* LSB */
    TH = ds18b20_read_byte(); /* MSB */

    /* 将 (TH << 8 | TL) 当作有符号 16-bit 原始值 */
    temp = (short)(((uint16_t)TH << 8U) | TL);

    /* 转换到 x10 格式：原始值 * 0.0625 * 10 = 原始值 * 10 / 16 */
    temp = (short)((int)temp * 10 / 16);

    return temp;
}

/**
* @brief 从 ds18b20 读取温度(精度： 0.1C) — 不启动转换，仅读取上次结果
* @param 无
* @retval 温度值 （-550~1250），通讯失败返回 0x7FFF
* @note 调用前需保证 ds18b20_start() 已执行且转换完成。
*      返回的温度值放大了 10 倍，实际使用时要除以 10 才是实际温度。
*/
short ds18b20_read_temperature(void)
{
    uint8_t TL, TH;
    short temp;

    ds18b20_reset();
    if (ds18b20_check() != 0)       /* 设备不存在或通讯异常 */
        return 0x7FFF;

    ds18b20_write_byte(0xcc); /* skip rom */
    ds18b20_write_byte(0xbe); /* read scratchpad */
    TL = ds18b20_read_byte(); /* LSB */
    TH = ds18b20_read_byte(); /* MSB */

    /* 将 (TH << 8 | TL) 当作有符号 16-bit 原始值 */
    temp = (short)(((uint16_t)TH << 8U) | TL);

    /* 转换到 x10 格式：原始值 * 0.0625 * 10 = 原始值 * 10 / 16 */
    temp = (short)((int)temp * 10 / 16);

    /* 二次校验：DS18B20 工作温度范围 -55°C ~ +125°C (-550 ~ 1250 in x10) */
    if (temp < -550 || temp > 1250)
        return 0x7FFF;

    return temp;
}
