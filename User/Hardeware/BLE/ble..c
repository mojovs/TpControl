#include <stdarg.h>

#include "ble.h"
#include "cmsis_os2.h"
#include "string.h"
#include "xprintf.h"
/**ble信息**/
/* addr 98daa000d449*/

UART_HandleTypeDef  ble_handle;


uint8_t  ble_rx_buffer[1] = {0};
uint8_t  ble_rx_flag = 0;
osMutexId_t mutex_ble;
const osMutexAttr_t mutex_ble_attributes = {
    .name = "mutex ble",
    .attr_bits = osMutexRecursive
};


void ble_init(uint32_t bound)
{
    mutex_ble= osMutexNew(&mutex_ble_attributes); //锁
    ble_handle.Instance = USART2;
    ble_handle.Init.BaudRate = bound;  //蓝牙为9600
    ble_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    ble_handle.Init.Mode = UART_MODE_TX_RX;
    //blert_handle.Init.OverSampling = ;  //过采样，不需要
    ble_handle.Init.Parity = UART_PARITY_NONE;//不需要奇偶校验
    ble_handle.Init.StopBits = UART_STOPBITS_1;
    ble_handle.Init.WordLength = UART_WORDLENGTH_8B;
    //初始化
    HAL_UART_Init(&ble_handle);
    //初始化串口2接收中断
    HAL_UART_Receive_IT(&ble_handle,ble_rx_buffer,1);


}

/* 串口1中断服务函数 */
void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&ble_handle);
    HAL_UART_Receive_IT(&ble_handle, (uint8_t*)ble_rx_buffer, 1);
}

void ble_send(char *fmt,...)
{
#define BLE
#ifdef BLE
    /* 调度器启动前跳过互斥锁 */
    osKernelState_t state = osKernelGetState();
    uint8_t kernel_running = (state == osKernelRunning);

    if (!kernel_running) {
        memset(send_data, 0, sizeof(send_data));
        va_list ap;
        va_start(ap, fmt);
        xsprintf_m(send_data, fmt, ap);
        va_end(ap);
        HAL_UART_Transmit(&ble_handle, (uint8_t*)send_data, strlen(send_data), 100);
        return;
    }

    osMutexAcquire(mutex_ble, osWaitForever);
    memset(send_data,0,sizeof(send_data));
    va_list ap;
    va_start(ap,fmt);
    xsprintf_m(send_data,fmt,ap);
    va_end(ap);
    HAL_UART_Transmit(&ble_handle, (uint8_t*)send_data, strlen(send_data), 100);
    osMutexRelease(mutex_ble);
#endif

}

void ble_send_m(char* fmt, va_list arg)
{
    osMutexAcquire(mutex_ble,osWaitForever);

    memset(send_data,0,sizeof(send_data));
    xsprintf_m(send_data,fmt,arg);
    HAL_UART_Transmit(&ble_handle, (uint8_t*)send_data, strlen(send_data), 100);
    osMutexRelease(mutex_ble);
}

void ble_receive(char *msg)
{
    if (ble_rx_flag == 1)
    {
    osMutexAcquire(mutex_ble,osWaitForever);
        HAL_UART_Receive_IT(&ble_handle, ble_rx_buffer, 1);
    osMutexRelease(mutex_ble);
    }
}
