//
// Created by meng on 2024/6/12.
//

#include "uart.h"

#include "app_config.h"

extern uint8_t ble_rx_flag;

#if ENABLE_UART1_DEBUG
#include <stdarg.h>
#include <string.h>

#include "cmsis_os2.h"
#include "xprintf.h"

#define UART1_RX_RING_SIZE 512

UART_HandleTypeDef  uart_handle;

uint8_t  uart1_rx_buffer[1] = {0};
uint8_t  uart1_rx_flag = 0;

static volatile uint16_t uart1_rx_head = 0;
static volatile uint16_t uart1_rx_tail = 0;
static uint8_t uart1_rx_ring[UART1_RX_RING_SIZE];

static void uart1_rx_push(uint8_t data)
{
    uint16_t next = (uint16_t)((uart1_rx_head + 1) % UART1_RX_RING_SIZE);

    if (next != uart1_rx_tail) {
        uart1_rx_ring[uart1_rx_head] = data;
        uart1_rx_head = next;
    }
}

void uart1_init(uint32_t bound)
{
    uart_handle.Instance = USART1;
    uart_handle.Init.BaudRate = bound;
    uart_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uart_handle.Init.Mode = UART_MODE_TX_RX;
    //uart_handle.Init.OverSampling = ;  //过采样，不需要
    uart_handle.Init.Parity = UART_PARITY_NONE;//不需要奇偶校验
    uart_handle.Init.StopBits = UART_STOPBITS_1;
    uart_handle.Init.WordLength = UART_WORDLENGTH_8B;
    //初始化
    HAL_UART_Init(&uart_handle);
    //初始化串口1接收中断
    HAL_UART_Receive_IT(&uart_handle,uart1_rx_buffer,1);

}
#endif

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef gpio_init_struct;
#if ENABLE_UART1_DEBUG
    if(huart->Instance == USART1)                /* 如果是串口1，进行串口1 MSP初始化 */
    {
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        gpio_init_struct.Pin = GPIO_PIN_9;
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;            /* 推挽式复用输出 */
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;      /* 高速 */
        HAL_GPIO_Init(GPIOA, &gpio_init_struct);            /* 初始化串口1的TX引脚 */

        gpio_init_struct.Pin = GPIO_PIN_10;
        gpio_init_struct.Mode = GPIO_MODE_AF_INPUT;         /* 输入 */
        gpio_init_struct.Pull = GPIO_PULLUP;                /* 上拉 */
        HAL_GPIO_Init(GPIOA, &gpio_init_struct);            /* 初始化串口1的RX引脚 */

        HAL_NVIC_SetPriority(USART1_IRQn, 6, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
    }
    else
#endif
    if(huart->Instance == USART2)                /* 如果是串口2，进行串口2 MSP初始化 */
    {
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        gpio_init_struct.Pin = GPIO_PIN_2;
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;            /* 推挽式复用输出 */
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;      /* 高速 */
        HAL_GPIO_Init(GPIOA, &gpio_init_struct);            /* 初始化串口2的TX引脚 */

        gpio_init_struct.Pin = GPIO_PIN_3;
        gpio_init_struct.Mode = GPIO_MODE_AF_INPUT;         /* 输入 */
        gpio_init_struct.Pull = GPIO_PULLUP;                /* 上拉 */
        HAL_GPIO_Init(GPIOA, &gpio_init_struct);            /* 初始化串口2的RX引脚 */

        HAL_NVIC_SetPriority(USART2_IRQn, 6, 0);
        HAL_NVIC_EnableIRQ(USART2_IRQn);
    }
}

#if ENABLE_UART1_DEBUG
/* 串口1中断服务函数 */
void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&uart_handle);
}
#endif

/* 串口数据接收完成回调函数 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
#if ENABLE_UART1_DEBUG
    if(huart->Instance == USART1)
    {
        uart1_rx_flag = 1;
        uart1_rx_push(uart1_rx_buffer[0]);
        HAL_UART_Receive_IT(&uart_handle, (uint8_t*)uart1_rx_buffer, 1);
    }
    else
#endif
    if(huart->Instance == USART2)
    {
        ble_rx_flag = 1;
    }
}

#if ENABLE_UART1_DEBUG
int uart1_read_byte(uint8_t *ch, uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();

    while (uart1_rx_head == uart1_rx_tail) {
        if (timeout_ms != HAL_MAX_DELAY && (HAL_GetTick() - start) >= timeout_ms) {
            return 0;
        }

        if (osKernelGetState() == osKernelRunning) {
            osDelay(1);
        }
    }

    *ch = uart1_rx_ring[uart1_rx_tail];
    uart1_rx_tail = (uint16_t)((uart1_rx_tail + 1) % UART1_RX_RING_SIZE);
    return 1;
}

int uart1_read(uint8_t *buf, uint32_t len, uint32_t timeout_ms)
{
    uint32_t i;

    for (i = 0; i < len; i++) {
        if (!uart1_read_byte(&buf[i], timeout_ms)) {
            return (int)i;
        }
    }

    return (int)i;
}

void uart1_send(const uint8_t *buf, uint32_t len)
{
    HAL_UART_Transmit(&uart_handle, (uint8_t*)buf, (uint16_t)len, HAL_MAX_DELAY);
}

void uart1_printf(char *fmt, ...)
{
    char data[128];
    va_list ap;

    memset(data, 0, sizeof(data));
    va_start(ap, fmt);
    xsprintf_m(data, fmt, ap);
    va_end(ap);

    uart1_send((uint8_t*)data, strlen(data));
}
#endif

