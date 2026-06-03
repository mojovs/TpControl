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
    uint8_t flag = 1; /* 默认温度为正数 */
    uint8_t TL, TH;
    short temp;
    ds18b20_start(); /* ds1820 start convert */
    ds18b20_reset();
    ds18b20_check();
    ds18b20_write_byte(0xcc); /* skip rom */
    ds18b20_write_byte(0xbe); /* convert */
    TL = ds18b20_read_byte(); /* LSB */
    TH = ds18b20_read_byte(); /* MSB */
    if (TH > 7)
    { /* 温度为负，查看 DS18B20 的温度表示法与计算机存储正负数据的原理一致：
    正数补码为寄存器存储的数据自身，负数补码为寄存器存储值按位取反后+1
    所以我们直接取它实际的负数部分，但负数的补码为取反后加一，但考虑到低位可能+1 后
    有进位和代码冗余，我们这里先暂时没有作+1 的处理，这里需要留意 */
        TH = ~TH;
        TL = ~TL;
        flag = 0;
    }
    temp = TH; /* 获得高八位 */
    temp <<= 8;
    temp += TL; /* 获得低八位 */
    /* 转换成实际温度 */
    if (flag == 0)
    { /* 将温度转换成负温度，这里的+1 参考前面的说明 */
        temp = (double)(temp+1) * 0.625;
        temp = -temp;
    } else
    {
        temp = (double)temp * 0.625;
    }
    return temp;
}
