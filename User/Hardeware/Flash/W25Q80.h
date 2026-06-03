//
// Created by Meng on 2025/11/23.
//

#ifndef FLASH_H
#define FLASH_H
#include "sys.h"
#include "spi.h"
#define W25Q80_WRITE_ENABLE							0x06
#define W25Q80_WRITE_DISABLE						0x04
#define W25Q80_READ_STATUS_REGISTER_1				0x05
#define W25Q80_READ_STATUS_REGISTER_2				0x35
#define W25Q80_WRITE_STATUS_REGISTER				0x01
#define W25Q80_PAGE_PROGRAM							0x02
#define W25Q80_QUAD_PAGE_PROGRAM					0x32
#define W25Q80_BLOCK_ERASE_64KB						0xD8
#define W25Q80_BLOCK_ERASE_32KB						0x52
#define W25Q80_SECTOR_ERASE_4KB						0x20
#define W25Q80_CHIP_ERASE							0xC7
#define W25Q80_ERASE_SUSPEND						0x75
#define W25Q80_ERASE_RESUME							0x7A
#define W25Q80_POWER_DOWN							0xB9
#define W25Q80_HIGH_PERFORMANCE_MODE				0xA3
#define W25Q80_CONTINUOUS_READ_MODE_RESET			0xFF
#define W25Q80_RELEASE_POWER_DOWN_HPM_DEVICE_ID		0xAB
#define W25Q80_MANUFACTURER_DEVICE_ID				0x90
#define W25Q80_READ_UNIQUE_ID						0x4B
#define W25Q80_JEDEC_ID								0x9F
#define W25Q80_READ_DATA							0x03
#define W25Q80_FAST_READ							0x0B
#define W25Q80_FAST_READ_DUAL_OUTPUT				0x3B
#define W25Q80_FAST_READ_DUAL_IO					0xBB
#define W25Q80_FAST_READ_QUAD_OUTPUT				0x6B
#define W25Q80_FAST_READ_QUAD_IO					0xEB
#define W25Q80_OCTAL_WORD_READ_QUAD_IO				0xE3

#define W25Q80_DUMMY_BYTE							0xFF
void w25q80_gpio_init();
void w25q80_cs_set();
void w25q80_cs_clear();

void w25q80_write_en();
u8 SPI_SwapByte(u8 sendByte);
void w25q80_init();
void w25q80_read_id(uint8_t *MID, uint16_t *DID);
void w25q80_page_program(uint32_t Address, uint8_t *DataArray, uint16_t Count);
void w25q80_sector_erase(uint32_t Address);
void w25q80_read_data(uint32_t Address, uint8_t *DataArray, uint32_t Count);
u8 w25q80_wait_busy(void);
#endif //FLASH_H

