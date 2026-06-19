#include "W25Q80.h"

#include <sys/stat.h>

#include "cmsis_os2.h"
#include "delay.h"
#include "lfs.h"
#include "MyTask.h"
#include "spi_sel.h"

void w25q80_gpio_init()
{
    //初始化片选引脚
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef gpio_initer;
    gpio_initer.Pin=GPIO_PIN_0;
    gpio_initer.Mode=GPIO_MODE_OUTPUT_PP;
    gpio_initer.Pull=GPIO_PULLUP;   //默认不选中
    gpio_initer.Speed=GPIO_SPEED_HIGH;
    HAL_GPIO_Init(GPIOB,&gpio_initer);
}


void w25q80_init()
{
    w25q80_gpio_init();  //

}
//向SPI总线传输一个8位数据
u8 SPI_SwapByte(u8 sendByte)
{

    u8 recvByte=0;  //接受的数据
    HAL_SPI_TransmitReceive(&spi_handler_1,&sendByte,&recvByte,1,10);
    return recvByte;
}


void w25q80_read_id(uint8_t* MID, uint16_t* DID)
{

     osMutexAcquire(mutex_spi1,osWaitForever);
    SPI1_Sel_Device(SPI1_FLASH);
    SPI_SwapByte(W25Q80_JEDEC_ID);
    *MID=SPI_SwapByte(W25Q80_DUMMY_BYTE);
    *DID=SPI_SwapByte(W25Q80_DUMMY_BYTE);
    //移到高位
    *DID<<=8;
    *DID|=SPI_SwapByte(W25Q80_DUMMY_BYTE);
    SPI1_Unsel_Device(SPI1_FLASH);
     osMutexRelease(mutex_spi1);
}

void w25q80_page_program(uint32_t Address, uint8_t* DataArray, uint16_t Count)
{
    u16 i=0;
     osMutexAcquire(mutex_spi1,osWaitForever);
    w25q80_write_en();
    SPI1_Sel_Device(SPI1_FLASH);
    SPI_SwapByte(W25Q80_PAGE_PROGRAM);
    SPI_SwapByte(Address>>16);
    SPI_SwapByte(Address>>8);
    SPI_SwapByte(Address);
    for(i=0;i<Count;i++)
    {
        SPI_SwapByte(DataArray[i]);
    }
    SPI1_Unsel_Device(SPI1_FLASH);
    //w25q80_wait_busy();
    osMutexRelease(mutex_spi1);
}

void w25q80_sector_erase(uint32_t Address)
{

     osMutexAcquire(mutex_spi1,osWaitForever);
    w25q80_write_en();
    SPI1_Sel_Device(SPI1_FLASH);
    SPI_SwapByte(W25Q80_SECTOR_ERASE_4KB);
    SPI_SwapByte(Address>>16);
    SPI_SwapByte(Address>>8);
    SPI_SwapByte(Address);
    SPI1_Unsel_Device(SPI1_FLASH);

    //w25q80_wait_busy();
     osMutexRelease(mutex_spi1);

}

void w25q80_read_data(uint32_t Address, uint8_t* DataArray, uint32_t Count)
{
    u32 i;

     osMutexAcquire(mutex_spi1,osWaitForever);
    SPI1_Sel_Device(SPI1_FLASH);
    SPI_SwapByte(W25Q80_READ_DATA);
    SPI_SwapByte(Address>>16);
    SPI_SwapByte(Address>>8);
    SPI_SwapByte(Address);
    for(i=0;i<Count;i++)
    {
        DataArray[i]=SPI_SwapByte(W25Q80_DUMMY_BYTE);
    }
    SPI1_Unsel_Device(SPI1_FLASH);
     osMutexRelease(mutex_spi1);

}

void w25q80_write_en(void)
{
    SPI1_Sel_Device(SPI1_FLASH);
    SPI_SwapByte(W25Q80_WRITE_ENABLE);
    SPI1_Unsel_Device(SPI1_FLASH);
}
u8 w25q80_wait_busy(void)
{
    uint8_t status = 0;
    uint32_t timeout = 500;  // 500ms timeout, one loop roughly 1ms

    for (;;)
    {
        osKernelState_t kernel_state = osKernelGetState();
        uint8_t kernel_running = (kernel_state == osKernelRunning);

        if (kernel_running) {
            osMutexAcquire(mutex_spi1, osWaitForever);
        }

        SPI1_Sel_Device(SPI1_FLASH);
        SPI_SwapByte(W25Q80_READ_STATUS_REGISTER_1);
        status = SPI_SwapByte(W25Q80_DUMMY_BYTE);
        SPI1_Unsel_Device(SPI1_FLASH);

        if (kernel_running) {
            osMutexRelease(mutex_spi1);
        }

        if ((status & 0x01) == 0) {
            return 0;
        }

        if (--timeout == 0) {
            ble_send("flash wait busy time out\n");
            return 1;
        }

        if (kernel_running) {
            osDelay(1);
        } else {
            delay_ms(1);
        }
    }
}

