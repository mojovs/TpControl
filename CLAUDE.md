# TpControl V01 — STM32 恒温恒湿器

## 项目概况

STM32F103C8T6 + FreeRTOS V10.0.1 + CMSIS-RTOS V2 温湿度控制系统。

| 资源 | 规格 |
|------|------|
| MCU | STM32F103C8T6 (Cortex-M3, 72MHz) |
| Flash | 64KB 内部 + 1MB W25Q80 (SPI NOR) |
| RAM | 20KB 内部, FreeRTOS Heap = 5KB |
| 文件系统 | littlefs v2.0.11 (LFS_THREADSAFE + 递归互斥锁) |
| 显示器 | TFT LCD 1.8" 128x160 (ST7735-like, SPI1 CS=PB7) |
| 温度 | DS18B20 (1-Wire, PA0) |
| 湿度 | AC 电容式 (ADC2 + TIM1 激励) |
| RTC | 内部 RTC (LSE 32.768KHz) |
| 无线 | BLE 模块 (USART1, 9600bps) |
| 输入 | Key A/B + 旋转编码器 EC11 |
| 输出 | 蜂鸣器 (PWM) + LED + 继电器(加热/风扇) |

## draw.io 路径

```
D:\Program Files\draw.io\draw.io.exe
```

导出用法（在 Bash 中执行）：
```bash
"D:\Program Files\draw.io\draw.io.exe" --export --format png --output doc/image/xxx.png doc/image/xxx.drawio
```

## 项目目录结构

```
TpControl_V01/
├── CLAUDE.md                    # 本文件
├── CMakeLists.txt               # 项目构建文件
├── STM32F103C8TX_FLASH.ld      # 链接脚本 (Flash 64K, RAM 20K)
├── TpControl.ioc                # STM32CubeMX 配置
│
├── Core/                        # HAL 核心代码
│   ├── Inc/                     #   HAL 配置头文件
│   ├── Src/                     #   HAL 初始化/中断/系统调用
│   └── Startup/                 #   启动文件 (startup_stm32f103c8tx.s)
│
├── Drivers/                     # CMSIS + STM32F1xx HAL Driver
│
├── Middlewares/
│   └── Third_Party/FreeRTOS/    # FreeRTOS V10.0.1 + CMSIS-RTOS V2
│
├── User/                        # 用户代码
│   ├── Hardeware/               #   外设驱动
│   │   ├── adc/                 #     ADC2 初始化
│   │   ├── BLE/                 #     BLE UART 收发
│   │   ├── Buzz/                #     蜂鸣器 PWM 控制
│   │   ├── Delay/               #     延时函数
│   │   ├── Flash/               #     W25Q80 SPI Flash 底层驱动
│   │   ├── Fs/                  #     littlefs + W25Q80 文件系统接口
│   │   ├── Humidity/            #     AC 电容式湿度传感器
│   │   ├── Key/                 #     按键 + EC11 编码器
│   │   ├── Lcd/                 #     TFT 显示驱动 + GUI + 字库 + 图片
│   │   │   ├── Lcd_Driver.c/h   #       SPI 写数据/命令底层
│   │   │   ├── Gui.c/h          #       绘图函数 (点/线/圆/字符/图片)
│   │   │   ├── Font.c/h         #       字库数组 (ASCII + 中文 + 数字)
│   │   │   └── Image.c/h        #       图标数组 (12x12/24x24)
│   │   ├── RTC/                 #     内部 RTC 驱动
│   │   ├── Spi/                 #     SPI1 初始化 + 片选管理
│   │   ├── Temperature/         #     DS18B20 1-Wire 驱动
│   │   ├── tim/                 #     定时器初始化
│   │   ├── Uart/                #     串口 + printf 重定向
│   │   ├── gpio.c/h             #     GPIO 初始化
│   │   └── led.c/h              #     LED 控制
│   │
│   └── System/                  #   系统层
│       ├── Tasks/               #     FreeRTOS 任务定义
│       │   ├── MyTask.c         #      任务函数 + 互斥锁 + 信号量
│       │   └── MyTask.h         #      外部声明
│       ├── main.c               #     初始化流程 + main
│       ├── FreeRTOSConfig.h     #     FreeRTOS 配置
│       ├── stm32f1xx_it.h       #     中断声明
│       ├── sys.c/h              #     系统时钟/配置
│       └── Stdio/               #     xprintf 轻量 printf
│
├── doc/                         # 文档
│   ├── 任务划分.md              #   5 任务架构文档
│   ├── littlefs_W25Q80_driver_analysis.md  # SPI 双锁竞态分析
│   ├── littlefs_W25Q80_fix_plan.md         # 修复计划
│   └── image/
│       ├── task_architecture.drawio        # 任务架构图源文件
│       └── task_architecture.png           # 导出图片
│
├── pcb/                         # PCB 设计文件
├── img/                         # 原理图/图片
├── Tools/                       # 工具
├── SI/                          # 信号完整性分析
└── PowerDesigner/               # 电源设计
```

## 任务架构 (5 任务)

| 任务 | 优先级 | 栈 | 职责 |
|------|--------|-----|------|
| rtcTask | Low+2 | 256B | RTC 秒中断 → 读时间 → 通知 guiTask |
| guiTask (HMI) | Low | 1536B | LCD 显示 + 背光 + littlefs 读字体/图片 |
| sensorTask | Normal | 512B | DS18B20 温度 + AC 湿度采集 |
| ctrlTask | Normal+1 | 512B | 控温/控湿/告警/模式管理 |
| commTask (BLE) | Low | 512B | BLE 遥测发送 + 命令接收解析 |

**同步方式：**
- `mutex_spi1` (递归锁) — 保护 SPI1 总线 (TFT + Flash)
- `mutex_lfs` (递归锁) — littlefs 锁 (应与 mutex_spi1 统一)
- `rtc_sema` — RTC 秒中断信号量
- `osThreadFlagsSet` — rtcTask → guiTask 通知

## 已知问题

1. **SPI1 双锁竞态 (P0)**: W25Q80 用 `mutex_lfs`, TFT 用 `mutex_spi1`, 共用 SPI1 总线会冲突。`w25q80_fs.c` 中的 LFS_LOCK 回调应改用 `mutex_spi1`。
2. **W25Q80 函数缺锁 (P0)**: `W25Q80.c` 中底层 SPI 函数的 `osMutexAcquire/Release` 被注释掉了。
3. **GUI 字体读取慢**: 每字符都做 lfs open/seek/read/close。优化方案：启动时一次性缓存 ASCII 字库到 RAM (~4.5KB)。

## 编译

CLion + cmake-build-debug-stm32，输出 `TpControl.bin` (当前 ~51KB)。
