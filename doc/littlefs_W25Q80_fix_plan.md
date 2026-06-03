# LittleFS + W25Q80 驱动整改方案 — 逐行修改清单

## 说明

本文档列出每项修改对应的文件、行号和具体代码变更。
基于 `doc/littlefs_W25Q80_driver_analysis.md` 的分析结论。

---

## 修改 1：统一 SPI1 总线锁（P0）

### 目标

TFT 和 Flash 的所有 SPI1 操作共用一把锁，消除竞态条件。

### 方案

将 `mutex_spi1` 作为 SPI1 总线的统一锁。littlefs 的 lock/unlock 回调改为操作 `mutex_spi1`，
W25Q80 底层函数也补上 `mutex_spi1`。

### 涉及文件

#### 文件 1：`User\System\Tasks\MyTask.c`

**创建 `mutex_spi1`（如尚未创建，确认已有）**

`MyTask.c:58-62` — 确认 `mutex_spi1` 已存在，若之前未创建则补上：

```c
osMutexId_t mutex_spi1;
const osMutexAttr_t mutex_spi1_attr = {
    .name = "Mutex_spi1",
    // .attr_bits = osMutexRecursive  // 如需要递归可取消注释
};
```

`MyTask.c:101-105` — 确认 `MX_FREERTOS_Init()` 中已创建：

```c
mutex_spi1 = osMutexNew(&mutex_spi1_attr);
```

> 经查代码已存在，无需新增。

#### 文件 2：`User\Hardeware\Fs\w25q80_fs.c`

**修改 `lfs_lock()` — 改用 `mutex_spi1`**

`w25q80_fs.c:67-75`，原代码：

```c
int lfs_lock(const struct lfs_config *c)
{
    if (osMutexAcquire(mutex_lfs, osWaitForever) != osOK)
    {
        LFS_TRACE("Mutex erro");
        return LFS_ERR_IO;
    }
    return LFS_ERR_OK;
}
```

改为：

```c
int lfs_lock(const struct lfs_config *c)
{
    if (osMutexAcquire(mutex_spi1, osWaitForever) != osOK)
    {
        LFS_TRACE("Mutex erro");
        return LFS_ERR_IO;
    }
    return LFS_ERR_OK;
}
```

**修改 `lfs_unlock()` — 改用 `mutex_spi1`**

`w25q80_fs.c:76-83`，原代码：

```c
int lfs_unlock(const struct lfs_config *c)
{
    if (osMutexRelease(mutex_lfs) != osOK)
    {
        return LFS_ERR_IO;
    }
    return LFS_ERR_OK;
}
```

改为：

```c
int lfs_unlock(const struct lfs_config *c)
{
    if (osMutexRelease(mutex_spi1) != osOK)
    {
        return LFS_ERR_IO;
    }
    return LFS_ERR_OK;
}
```

**修改 `w25q80_sync_fs()` — 去掉自加锁**

`w25q80_fs.c:54-63`，原代码：

```c
int w25q80_sync_fs(const struct lfs_config* c)
{
    osMutexAcquire(mutex_lfs, osWaitForever);
    SPI1_Sel_Device(SPI1_FLASH);
    w25q80_wait_busy();
    SPI1_Unsel_Device(SPI1_FLASH);
    osMutexRelease(mutex_lfs);
    return LFS_ERR_OK;
}
```

改为：

```c
int w25q80_sync_fs(const struct lfs_config* c)
{
    // 此函数在 LFS_LOCK 内被调用，锁已持有
    SPI1_Sel_Device(SPI1_FLASH);
    w25q80_wait_busy();
    SPI1_Unsel_Device(SPI1_FLASH);
    return LFS_ERR_OK;
}
```

**修改 `w25q80_init_fs()` — 改用 `mutex_spi1`**

`w25q80_fs.c:85-105`，原代码：

```c
void w25q80_init_fs()
{
    osMutexAcquire(mutex_lfs, osWaitForever);
    int err = lfs_mount(&lfs, &cfg_w25q80_fs);
    if (err) {
        delay_ms(1000);
        lfs_format(&lfs, &cfg_w25q80_fs);
        err = lfs_mount(&lfs, &cfg_w25q80_fs);
        if (err) {
            while(1);
        }
    }
    osMutexRelease(mutex_lfs);
}
```

改为：

```c
void w25q80_init_fs()
{
    osMutexAcquire(mutex_spi1, osWaitForever);
    int err = lfs_mount(&lfs, &cfg_w25q80_fs);
    if (err) {
        delay_ms(1000);
        lfs_format(&lfs, &cfg_w25q80_fs);
        err = lfs_mount(&lfs, &cfg_w25q80_fs);
        if (err) {
            while(1);
        }
    }
    osMutexRelease(mutex_spi1);
}
```

#### 文件 3：`User\System\Tasks\MyTask.h`

**修改 `mutex_lfs` 外部声明为 `mutex_spi1`（可选，如不再需要 `mutex_lfs`）**

`MyTask.h:18`：

```c
extern osMutexId_t mutex_lfs;   // 给 littlefs 系统用的锁
```

可删除或保留（如其他模块还在用 `mutex_lfs`）。建议保留 `mutex_lfs` 声明，
因为 `humidityACTask` 等仍可能依赖。

---

## 修改 2：W25Q80 底层函数补上 `mutex_spi1` 锁（P0）

### 目标

`W25Q80.c` 中所有被注释掉的锁操作恢复，避免 lfs 外部直接调用时无保护。

### 涉及文件

#### 文件：`User\Hardeware\Flash\W25Q80.c`

> 以下每个函数中的 `osMutexAcquire/Release(mutex_spi1, ...)` 均已存在但被注释，
> 取消注释即可。

**`w25q80_read_id()` — `W25Q80.c:41,50`**

```c
void w25q80_read_id(uint8_t* MID, uint16_t* DID)
{
    // ↓ 取消注释
    osMutexAcquire(mutex_spi1, osWaitForever);
    SPI1_Sel_Device(SPI1_FLASH);
    // ... SPI 操作 ...
    SPI1_Unsel_Device(SPI1_FLASH);
    // ↓ 取消注释
    osMutexRelease(mutex_spi1);
}
```

**`w25q80_page_program()` — `W25Q80.c:56,69`**

```c
void w25q80_page_program(...)
{
    // ↓ 取消注释
    osMutexAcquire(mutex_spi1, osWaitForever);
    w25q80_write_en();
    // ... SPI 操作 ...
    w25q80_wait_busy();
    // ↓ 取消注释
    osMutexRelease(mutex_spi1);
}
```

**`w25q80_sector_erase()` — `W25Q80.c:75,85`**

```c
void w25q80_sector_erase(uint32_t Address)
{
    // ↓ 取消注释
    osMutexAcquire(mutex_spi1, osWaitForever);
    w25q80_write_en();
    // ... SPI 操作 ...
    w25q80_wait_busy();
    // ↓ 取消注释
    osMutexRelease(mutex_spi1);
}
```

**`w25q80_read_data()` — `W25Q80.c:93,104`**

```c
void w25q80_read_data(...)
{
    // ↓ 取消注释
    osMutexAcquire(mutex_spi1, osWaitForever);
    SPI1_Sel_Device(SPI1_FLASH);
    // ... SPI 操作 ...
    SPI1_Unsel_Device(SPI1_FLASH);
    // ↓ 取消注释
    osMutexRelease(mutex_spi1);
}
```

#### 需要添加头文件引用

`W25Q80.c:5-6` 当前已有：

```c
#include "cmsis_os2.h"
#include "lfs.h"
#include "MyTask.h"
```

确认 `MyTask.h` 已包含（声明了 `mutex_spi1`），如编译报错未定义则加入：

```c
extern osMutexId_t mutex_spi1;
```

---

## 修改 3：消除 prog/erase 路径中的冗余 wait_busy（P1）

### 目标

去除 `w25q80_page_program()` 和 `w25q80_sector_erase()` 中的 `w25q80_wait_busy()`，
统一由 `w25q80_sync_fs()` 等待。

### 涉及文件

#### 文件：`User\Hardeware\Flash\W25Q80.c`

**`w25q80_page_program()` — 删除 `w25q80_wait_busy()`**

`W25Q80.c:68`：

```c
void w25q80_page_program(uint32_t Address, uint8_t* DataArray, uint16_t Count)
{
    u16 i = 0;
    osMutexAcquire(mutex_spi1, osWaitForever);
    w25q80_write_en();
    SPI1_Sel_Device(SPI1_FLASH);
    SPI_SwapByte(W25Q80_PAGE_PROGRAM);
    SPI_SwapByte(Address >> 16);
    SPI_SwapByte(Address >> 8);
    SPI_SwapByte(Address);
    for (i = 0; i < Count; i++) {
        SPI_SwapByte(DataArray[i]);
    }
    SPI1_Unsel_Device(SPI1_FLASH);
    w25q80_wait_busy();   // ← 删除这行（移至 sync_fs 中等待）
    osMutexRelease(mutex_spi1);
}
```

**`w25q80_sector_erase()` — 删除 `w25q80_wait_busy()`**

`W25Q80.c:84`：

```c
void w25q80_sector_erase(uint32_t Address)
{
    osMutexAcquire(mutex_spi1, osWaitForever);
    w25q80_write_en();
    SPI1_Sel_Device(SPI1_FLASH);
    SPI_SwapByte(W25Q80_SECTOR_ERASE_4KB);
    SPI_SwapByte(Address >> 16);
    SPI_SwapByte(Address >> 8);
    SPI_SwapByte(Address);
    SPI1_Unsel_Device(SPI1_FLASH);
    w25q80_wait_busy();   // ← 删除这行（移至 sync_fs 中等待）
    osMutexRelease(mutex_spi1);
}
```

> 这两个函数中的 `w25q80_wait_busy()` 删除后，等待逻辑移到 `w25q80_sync_fs()` 中执行。
> `w25q80_sync_fs()` 由 lfs 内部在关键节点调用，已包含 `w25q80_wait_busy()`。

---

## 修改 4：`w25q80_wait_busy()` 加入 `osDelay` 避免饿死（P2）

### 目标

在忙等待轮询循环中加入 `osDelay(1)`，让出 CPU 给其他任务。

### 涉及文件

#### 文件：`User\Hardeware\Flash\W25Q80.c`

**`w25q80_wait_busy()` — 加入 `osDelay` 和基于 tick 的超时**

`W25Q80.c:114-134`，原代码：

```c
u8 w25q80_wait_busy(void)
{
    uint8_t status;
    uint32_t timeout = 300000;

    SPI1_Sel_Device(SPI1_FLASH);
    SPI_SwapByte(W25Q80_READ_STATUS_REGISTER_1);
    do {
        status = SPI_SwapByte(W25Q80_DUMMY_BYTE);
        if (--timeout == 0)
        {
            SPI1_Unsel_Device(SPI1_FLASH);
            ble_send("flash wait busy time out\n");
            return 1;
        }
    } while (status & 0x01);

    SPI1_Unsel_Device(SPI1_FLASH);
    return status & 0x01;
}
```

改为：

```c
u8 w25q80_wait_busy(void)
{
    uint8_t status;
    TickType_t startTick = xTaskGetTickCount();
    const TickType_t timeoutTicks = pdMS_TO_TICKS(1000);  // 1 秒超时

    SPI1_Sel_Device(SPI1_FLASH);
    SPI_SwapByte(W25Q80_READ_STATUS_REGISTER_1);
    do {
        status = SPI_SwapByte(W25Q80_DUMMY_BYTE);
        if (status & 0x01)
        {
            if ((xTaskGetTickCount() - startTick) >= timeoutTicks)
            {
                SPI1_Unsel_Device(SPI1_FLASH);
                ble_send("flash wait busy time out\n");
                return 1;
            }
            osDelay(1);   // 让出 CPU
        }
    } while (status & 0x01);

    SPI1_Unsel_Device(SPI1_FLASH);
    return 0;
}
```

---

## 修改 5：清理 LCD 废弃宏定义（P3）

### 目标

删除 `Lcd_Driver.h` 中已废弃的、与 PB0 冲突的 `LCD_CS_SET/CLR` 宏，
避免未来误用。

### 涉及文件

#### 文件：`User\Hardeware\Lcd\Lcd_Driver.h`

**删除或注释冲突的 CS 宏**

`Lcd_Driver.h:62,69-70,73,79-80,88-96`，以下内容可删除：

```c
// 以下三行中的 LCD_CS 与 Flash CS(PB0) 冲突，由 spi_sel.c 替代
#define LCD_CS        	GPIO_PIN_0  // MCU_PA0--->>TFT --CS/CE   ← 实际是 PB0

#define	LCD_CS_SET  	LCD_CTRLB->BSRR=LCD_CS     // ← 删除
#define	LCD_CS_CLR  	LCD_CTRLB->BRR=LCD_CS      // ← 删除
```

以及 `Lcd_Driver.h:88-96` 中的 `LCD_WR_DATA` 宏：

```c
// 以下宏为旧版并口写模式，在此项目中未使用，可删除
#define LCD_WR_DATA(data){\
    LCD_RS_SET;\
    LCD_CS_CLR;\
    LCD_DATAOUT(data);\
    LCD_WR_CLR;\
    LCD_WR_SET;\
    LCD_CS_SET;\
}
```

---

## 修改 6：`Lcd_ReadPoint` 修复或删除（P3）

### 涉及文件

#### 文件：`User\Hardeware\Lcd\Lcd_Driver.c`

**`Lcd_ReadPoint()` — 删除或实现**

`Lcd_Driver.c:307-318`，原代码：

```c
unsigned int Lcd_ReadPoint(u16 x, u16 y)
{
    unsigned int Data;
    Lcd_SetXY(x, y);
    Lcd_WriteData(Data);
    osMutexRelease(mutex_spi1);
    return Data;
}
```

如不需要读点功能，直接删除该函数（同时删除 `Lcd_Driver.h:116` 的声明）。

如需保留，需要实现 SPI 全双工读取 LCD GRAM（需查 ST7735 读数据时序），大致框架：

```c
unsigned int Lcd_ReadPoint(u16 x, u16 y)
{
    unsigned int Data = 0;
    uint8_t rx[2] = {0};
    osMutexAcquire(mutex_spi1, osWaitForever);
    Lcd_SetRegion(x, y, x + 1, y + 1);
    Lcd_WriteIndex(0x2E);           // ST7735 读 RAM 指令
    // 需要先假读一个字节（dummy）
    HAL_SPI_Receive(&spi_handler_1, rx, 1, 10);
    // 真正读 2 字节（RGB565）
    HAL_SPI_Receive(&spi_handler_1, rx, 2, 10);
    Data = ((unsigned int)rx[0] << 8) | rx[1];
    osMutexRelease(mutex_spi1);
    return Data;
}
```

---

## 修改 7：删除 main.c 中重复的头文件包含（P3）

### 涉及文件

#### 文件：`User\System\main.c`

`main.c:21,28` — 同一个头文件引用了两次，删掉一行：

```c
#include "w25q80_fs.h"    // ← 保留这行
#include "Fs/w25q80_fs.h" // ← 删除这行
```

---

## 修改对照总表

| # | 文件 | 行号 | 操作 | 描述 |
|---|------|------|------|------|
| 1a | `w25q80_fs.c` | 70 | 修改 | `lfs_lock()` 中 `mutex_lfs` → `mutex_spi1` |
| 1b | `w25q80_fs.c` | 81 | 修改 | `lfs_unlock()` 中 `mutex_lfs` → `mutex_spi1` |
| 1c | `w25q80_fs.c` | 56-61 | 修改 | `w25q80_sync_fs()` 去掉 `osMutexAcquire/Release(mutex_lfs)` |
| 1d | `w25q80_fs.c` | 90,104 | 修改 | `w25q80_init_fs()` 中 `mutex_lfs` → `mutex_spi1` |
| 2a | `W25Q80.c` | 41,50 | 取消注释 | `w25q80_read_id()` 恢复 `osMutexAcquire/Release(mutex_spi1)` |
| 2b | `W25Q80.c` | 56,69 | 取消注释 | `w25q80_page_program()` 恢复 `osMutexAcquire/Release(mutex_spi1)` |
| 2c | `W25Q80.c` | 75,85 | 取消注释 | `w25q80_sector_erase()` 恢复 `osMutexAcquire/Release(mutex_spi1)` |
| 2d | `W25Q80.c` | 93,104 | 取消注释 | `w25q80_read_data()` 恢复 `osMutexAcquire/Release(mutex_spi1)` |
| 3a | `W25Q80.c` | 68 | 删除 | `w25q80_page_program()` 中 `w25q80_wait_busy()` |
| 3b | `W25Q80.c` | 84 | 删除 | `w25q80_sector_erase()` 中 `w25q80_wait_busy()` |
| 4 | `W25Q80.c` | 114-134 | 重写 | `w25q80_wait_busy()` 改为基于 tick 超时 + `osDelay(1)` |
| 5 | `Lcd_Driver.h` | 62,69,73,79,88-96 | 删除 | 废弃的 `LCD_CS_SET/CLR`、`LCD_WR_DATA` 宏 |
| 6 | `Lcd_Driver.c` | 307-318 | 删除或重写 | `Lcd_ReadPoint()` bug 修复 |
| 7 | `main.c` | 21 或 28 | 删除 | 重复的 `#include "Fs/w25q80_fs.h"` |
