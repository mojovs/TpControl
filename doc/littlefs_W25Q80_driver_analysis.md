# LittleFS + W25Q80 驱动 FreeRTOS 下问题分析与改进方案

## 概述

本文分析 littlefs 文件系统与 W25Q80 SPI Flash 在 STM32F103C8T6 + FreeRTOS 环境下的驱动代码，
排查多任务并发访问 SPI1 总线时的竞态条件、冗余操作和架构设计问题。

---

## 1. SPI1 总线竞态 — 双锁问题（严重）

### 问题描述

SPI1 总线上挂载了两个设备：

| 设备 | CS 引脚 | 驱动文件 | 使用的互斥锁 |
|------|---------|----------|-------------|
| TFT 屏幕 | PB7 | `Lcd_Driver.c` | `mutex_spi1` |
| W25Q80 Flash | PB0 | `W25Q80.c` / `w25q80_fs.c` | `mutex_lfs` |

TFT 和 Flash 使用 **两个不同的互斥锁** 来保护对同一套 SPI1 外设寄存器的访问。

### 风险

任务 A 持有 `mutex_spi1` 对 TFT 发起 SPI 传输（`HAL_SPI_Transmit`），
任务 B 可以同时持有 `mutex_lfs` 对 Flash 发起 SPI 传输（`HAL_SPI_TransmitReceive`）。

SPI1 外设寄存器（DR、CR1、SR 等）只有一套，两个传输同时进行会导致数据错乱。

### 根因

```c
// Lcd_Driver.c — TFT 写数据
void SPI_WriteData(u8 data) {
    HAL_SPI_Transmit(&spi_handler_1, &data, 1, 10);  // 受 mutex_spi1 保护
}

// W25Q80.c — Flash 收发字节
u8 SPI_SwapByte(u8 sendByte) {
    HAL_SPI_TransmitReceive(&spi_handler_1, &sendByte, &recvByte, 1, 10);  // 受 mutex_lfs 保护
}
```

### 改进方案

为 SPI1 引入统一的访问锁，所有 SPI1 操作共用同一把锁。

**有两种实现方式**：

**方案 A**：将 `mutex_spi1` 作为底层锁，LFS_LOCK/LFS_UNLOCK 内部也使用它。

```
LFS_LOCK → acquire(mutex_spi1)
  → cfg->read/prog/erase → SPI 操作 (已持有 mutex_spi1)
LFS_UNLOCK → release(mutex_spi1)
```

这样 TFT 操作和 Flash 操作都经过同一把锁，不会并行。

**方案 B**：新增一把 `mutex_spi1_bus`，在 `SPI1_Sel_Device()` 和 `SPI1_Unsel_Device()` 中获取/释放，让 SPI 设备选择与总线锁定绑定。

> 推荐方案 A，改动最小且与现有 `spi_sel` 机制兼容。

---

## 2. W25Q80 底层函数注释掉了互斥锁（严重）

### 问题描述

`W25Q80.c` 中所有底层 SPI 操作函数的互斥锁获取/释放都被注释掉了：

```c
void w25q80_read_data(...) {
    // osMutexAcquire(mutex_spi1, osWaitForever);  ← 注释
    SPI1_Sel_Device(SPI1_FLASH);
    // ... SPI 操作 ...
    SPI1_Unsel_Device(SPI1_FLASH);
    // osMutexRelease(mutex_spi1);                  ← 注释
}
```

受影响的函数：

| 函数 | 文件位置 |
|------|----------|
| `w25q80_read_id` | W25Q80.c:38-51 |
| `w25q80_page_program` | W25Q80.c:53-70 |
| `w25q80_sector_erase` | W25Q80.c:72-87 |
| `w25q80_read_data` | W25Q80.c:89-106 |
| `w25q80_write_en` | W25Q80.c:108-113 |
| `w25q80_wait_busy` | W25Q80.c:114-134 |

### 风险

这些函数目前仅靠 LFS_LOCK（`mutex_lfs`）间接保护。但如果被 lfs 之外的代码直接调用
（如 `main.c` 中的 `flash_test()`、`w25q80_read_id()` 在初始化阶段调用），
就没有任何锁机制，会与 TFT 操作冲突。

### 改进方案

- 方案 A 实施后（统一 SPI1 锁），在这些函数入口/出口加入 `osMutexAcquire/Release(mutex_spi1)`。
- 或者，将 SPI1 锁定下沉到 `SPI_SwapByte()` 函数级别，但粒度过细性能差，不推荐。

---

## 3. `w25q80_sync_fs()` 中多余的递归加锁（中）

### 问题描述

调用链分析：

```
lfs 内部操作
  → LFS_LOCK(cfg)  → lfs_lock()  → osMutexAcquire(mutex_lfs)   ← LOCK-1
  → cfg->prog()     → w25q80_pro_fs()
    → w25q80_page_program()
      → w25q80_wait_busy()          // 等 flash 空闲
    → w25q80_sync_fs(c)
      → osMutexAcquire(mutex_lfs)   ← LOCK-2 (递归锁)
      → w25q80_wait_busy()          // 再等一次
      → osMutexRelease(mutex_lfs)   ← UNLOCK-2
  → LFS_UNLOCK(cfg) → lfs_unlock()  → osMutexRelease(mutex_lfs) ← UNLOCK-1
```

`w25q80_sync_fs()` 获取的 `mutex_lfs` 与 LFS_LOCK 已持有的锁是同一个。
代码依赖 `osMutexRecursive` 来避免死锁。

### 风险

- 每次 sync 多两次无意义的锁操作（acquire + release），增加开销。
- 架构上不清晰——回调函数不应该知道自己被锁包裹。
- 如果未来改用非递归锁，这里会直接死锁。

### 改进方案

`w25q80_sync_fs()` 中不获取/释放 `mutex_lfs`，只执行 Sync 本身的逻辑：

```c
int w25q80_sync_fs(const struct lfs_config *c) {
    // 注意：此函数在 LFS_LOCK 内被调用，不需要再获取 mutex_lfs
    SPI1_Sel_Device(SPI1_FLASH);
    w25q80_wait_busy();
    SPI1_Unsel_Device(SPI1_FLASH);
    return LFS_ERR_OK;
}
```

---

## 4. 重复的忙等待（中）

### 问题描述

`w25q80_pro_fs()` 和 `w25q80_erase_fs()` 中都调用了两次 `w25q80_wait_busy()`。

**prog 路径**：

```c
int w25q80_pro_fs(...) {
    w25q80_page_program(c->block_size*block+off, (u8*)buffer, size);
    //   ↑ 内部调用 w25q80_wait_busy()
    w25q80_sync_fs(c);
    //   ↑ 内部又调用 w25q80_wait_busy()  ← 冗余
    return LFS_ERR_OK;
}
```

**erase 路径**：

```c
int w25q80_erase_fs(...) {
    w25q80_sector_erase(block<<12);
    //   ↑ 内部调用 w25q80_wait_busy()
    w25q80_sync_fs(c);
    //   ↑ 内部又调用 w25q80_wait_busy()  ← 冗余
    return LFS_ERR_OK;
}
```

### 影响

每次写/擦除操作需多等待一次 Flash 操作周期（W25Q80 page program 典型 0.7ms，sector erase 典型 45ms），
累积下来降低文件系统吞吐量。

### 改进方案

两种方法（选一种）：

1. 从 `w25q80_page_program()` 和 `w25q80_sector_erase()` 中移除 `w25q80_wait_busy()`，
   统一放到 `w25q80_sync_fs()` 中等待。
2. 从 `w25q80_pro_fs()` 和 `w25q80_erase_fs()` 中移除 `w25q80_sync_fs()` 调用，
   让 sync 仅由 lfs 内部在适当时机调用。

推荐方法 1，因为 lfs 内部 sync 调用频繁，不应每次都 busy-wait 等待。

---

## 5. `w25q80_wait_busy()` 纯 spin-lock 轮询（中）

### 问题描述

```c
u8 w25q80_wait_busy(void) {
    uint32_t timeout = 300000;
    ...
    do {
        status = SPI_SwapByte(W25Q80_DUMMY_BYTE);
        if (--timeout == 0) { ... return 1; }
    } while (status & 0x01);
    ...
}
```

30 万次循环的纯 spin-lock，轮询间隙没有让出 CPU。

### 影响

在 FreeRTOS 中，这个函数执行期间（sector erase 最长 ~100ms + 轮询开销），
当前任务一直占用 CPU，高优先级的任务（如 `humidityACTask`）得不到调度。

如果 `mutex_lfs` 或 `mutex_spi1` 被持有，其他需要 SPI1 的任务也会阻塞。

### 改进方案

在轮询循环中加入 `osDelay(1)` 或 `taskYIELD()`：

```c
do {
    status = SPI_SwapByte(W25Q80_DUMMY_BYTE);
    if (status & 0x01) {
        osDelay(1);     // 让出 CPU，等 1ms 再查
        if (--timeout == 0) { ... return 1; }
    }
} while (status & 0x01);
```

也可以使用 FreeRTOS 的 `xTaskGetTickCount()` 实现超时，以 tick 为单位而非循环次数，
使超时时间与系统时钟挂钩，不受 CPU 频率影响。

---

## 6. `LFS_LOCK`/`LFS_UNLOCK` 不检查返回值（低）

### 问题描述

```c
// lfs.c:5963-5965
#ifdef LFS_THREADSAFE
#define LFS_LOCK(cfg)   cfg->lock(cfg)    // 返回值被丢弃
#define LFS_UNLOCK(cfg) cfg->unlock(cfg)  // 返回值被丢弃
```

虽然 `lfs_lock()` 和 `lfs_unlock()` 会返回错误码，但宏定义直接丢弃了返回值。

### 影响

如果 `osMutexAcquire` 返回错误（如 `osErrorResource`），lfs 依然继续执行 SPI 操作，
可能操作未保护的硬件或产生未定义行为。

### 改进方案

在 LFS_LOCK 宏中检查返回值，失败则返回错误：

```c
#define LFS_LOCK(cfg)   do { \
    int _err = cfg->lock(cfg); \
    if (_err) return _err; \
} while (0)
```

> 注意：需要在 lfs.c 中的每个 public API 函数入口确保 LFS_LOCK 不放在 `return` 语句中
>（当前 lfs.c 的 LFS_LOCK 调用模式是 `LFS_LOCK(cfg); ...; LFS_UNLOCK(cfg); return err;`，
> 改成 do-while 宏需要评估影响，也可以暂时不做）。

---

## 7. SPI 设备选择与旧版 LCD 宏定义冲突（低）

### 问题描述

`Lcd_Driver.h` 中有两套 CS 控制机制：

**新机制（`spi_sel.c`）**：

```c
#define SPI_W25Q80_PIN GPIO_PIN_0   // PB0 → Flash CS
#define SPI_TFT_PIN    GPIO_PIN_7   // PB7 → TFT CS
```

**旧宏（`Lcd_Driver.h`）**：

```c
#define LCD_CS  GPIO_PIN_0          // PB0 → 旧 TFT CS（与 Flash CS 冲突！）
#define LCD_CS_SET  LCD_CTRLB->BSRR = LCD_CS
#define LCD_CS_CLR  LCD_CTRLB->BRR  = LCD_CS
```

### 影响

- 旧宏 `LCD_CS_SET/CLR` 操作的是 PB0，与 Flash 片选引脚相同
- 如果某处代码错误地使用了这些旧宏，会让 Flash 和 TFT 的 CS 互相干扰
- 目前实际使用的 SPI 选择机制（`SPI1_Sel_Device`）通过 PB0/PB7 区分设备，工作正常

### 改进方案

清理 `Lcd_Driver.h` 中废弃的 `LCD_CS_SET/CLR` 宏及相关位带操作宏，避免后续误用。

---

## 8. `Lcd_ReadPoint` 函数存在 bug（低）

### 问题描述

```c
unsigned int Lcd_ReadPoint(u16 x, u16 y) {
    unsigned int Data;        // ← 未初始化
    Lcd_SetXY(x, y);
    Lcd_WriteData(Data);      // ← 把未初始化的 Data 写入了 LCD
    osMutexRelease(mutex_spi1);
    return Data;              // ← 返回未初始化的值
}
```

该函数本应从 LCD GRAM 读回像素颜色，但代码实际是写入了一个未初始化的值并把它返回。

### 改进方案

- 如果不需要读点功能，删除或注释掉该函数
- 如果需要，使用 SPI 全双工（`HAL_SPI_TransmitReceive`）正确实现 LCD 读数据时序

---

## 总结

### 修复优先级

| 优先级 | 问题 | 涉及文件 | 影响 |
|--------|------|----------|------|
| **P0** | SPI1 双锁竞态（#1） | `Lcd_Driver.c`, `w25q80_fs.c`, `W25Q80.c` | SPI 数据错乱，系统不稳定 |
| **P0** | W25Q80 函数缺锁（#2） | `W25Q80.c` | lfs 外直接调用无保护 |
| **P1** | sync_fs 递归锁（#3） | `w25q80_fs.c` | 架构隐患，未来换非递归锁死锁 |
| **P1** | 冗余 wait_busy（#4） | `w25q80_fs.c`, `W25Q80.c` | 每次写/擦多等一倍时间 |
| **P2** | spin-lock 轮询（#5） | `W25Q80.c` | 高优先级任务被阻塞 |
| **P3** | LFS_LOCK 不检返回值（#6） | `lfs.c` (第三方库) | 极端情况锁失败但继续操作 |
| **P3** | 旧宏冲突（#7） | `Lcd_Driver.h` | 代码维护隐患 |
| **P3** | Lcd_ReadPoint bug（#8） | `Lcd_Driver.c` | 读点功能不可用 |

### 关键修改建议

1. **统一 SPI1 锁**：让 flash 操作和 TFT 操作共用 `mutex_spi1`，或建立 SPI1 总线级互斥
2. **取消 sync_fs 自加锁**：回调函数不自行加锁，锁由 LFS_LOCK 统一管理
3. **消除重复等待**：prog/erase 后只等一次 busy
4. **轮询加入 osDelay**：避免 busy-wait 饿死其他任务
