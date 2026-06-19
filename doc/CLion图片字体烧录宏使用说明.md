# CLion 图片/字体烧录宏使用说明

本文说明如何在 CLion 环境下，通过 CMake 宏把图片、字体资源临时编进固件，然后由程序写入 W25Q80 的 littlefs 文件系统。

> 烧录完成并确认资源已写入后，建议把这些宏关掉重新编译正常固件，避免资源数组继续占用 STM32 内部 Flash。

## 1. 烧录入口

当前资源写入入口在 `User/System/main.c` 初始化流程中：

```c
w25q80_init_fs(); // 初始化文件系统
write_images();   // 按宏写入图片资源
```

字体写入函数在 `User/Hardeware/Lcd/Font.c` 中，例如：

```c
lfs_write_GB12();
lfs_write_asc_1206();
lfs_write_asc_1608();
lfs_write_sz_32();
```

如需烧录字体，需要在初始化 littlefs 之后临时调用对应函数。烧录完成后再注释掉调用。

## 2. CLion 中设置 CMake 宏

在 CLion 中操作：

1. 打开 `Settings / Preferences`
2. 进入 `Build, Execution, Deployment` → `CMake`
3. 选择当前 Profile，例如 `Debug-stm32`
4. 在 `CMake options` 中添加或修改宏
5. 点击 `Apply`
6. 重新 Reload CMake / Build
7. 下载固件到板子运行，让程序执行资源写入
8. 写入完成后关闭宏，重新编译正常固件

示例：

```text
-DWRITE_IMAGE_GROUP=12x12 -DWRITE_FONT_GB12=ON
```

多个宏之间用空格分隔。

## 3. 图片烧录宏

图片资源由 `CMakeLists.txt` 中的 `WRITE_IMAGE_GROUP` 控制。

可选值：

| 宏 | 作用 |
|---|---|
| `-DWRITE_IMAGE_GROUP=none` | 不编译、不写入图片资源 |
| `-DWRITE_IMAGE_GROUP=12x12` | 编译 `img/icon_12x12/*.c`，写入 12x12 图标 |
| `-DWRITE_IMAGE_GROUP=24x24` | 编译 `img/icon_24x24/*.c`，写入 24x24 图标 |
| `-DWRITE_IMAGE_GROUP=24x48` | 编译 `img/icon_24x48/*.c`，写入 24x48 图标 |

默认值目前是：

```cmake
WRITE_IMAGE_GROUP=12x12
```

对应写入函数：

| 分组 | 代码函数 | 写入目录 |
|---|---|---|
| `12x12` | `write_icon_12x12()` | `/images/*.bin` |
| `24x24` | `write_icon_24x24()` | `/images/*.bin` |
| `24x48` | `write_icon_24x48()` | `/images/*.bin` |

### 3.1 烧录 12x12 图片

CLion CMake options：

```text
-DWRITE_IMAGE_GROUP=12x12
```

重新编译下载后，程序运行到 `write_images()` 会写入：

```text
/images/add_12x12.bin
/images/clock_12x12.bin
/images/fant_12x12.bin
/images/hot_12x12.bin
/images/hum_12x12.bin
/images/leftArrow_12x12.bin
/images/reduce_12x12.bin
/images/rightArrow_12x12.bin
/images/set_12x12.bin
```

### 3.2 烧录 24x24 图片

CLion CMake options：

```text
-DWRITE_IMAGE_GROUP=24x24
```

会写入：

```text
/images/bean_24x24.bin
/images/bread_24x24.bin
/images/chicken_24x24.bin
/images/egg_24x24.bin
/images/fan_24x24.bin
/images/heat_24x24.bin
/images/hum_24x24.bin
/images/printer_24x24.bin
/images/start_24x24.bin
/images/temp_24x24.bin
/images/temp_curve_24x24.bin
```

### 3.3 烧录 24x48 图片

CLion CMake options：

```text
-DWRITE_IMAGE_GROUP=24x48
```

会写入：

```text
/images/mode_button_48x24.bin
/images/start_button_48x24.bin
/images/stop_button_48x24.bin
```

### 3.4 关闭图片烧录

正常运行固件建议设置：

```text
-DWRITE_IMAGE_GROUP=none
```

这样不会把图片数组编进固件，也不会执行具体图片写入。

## 4. 字体烧录宏

字体资源由下面两个宏控制：

| 宏 | 作用 | 当前资源文件 | 写入目标 |
|---|---|---|---|
| `-DWRITE_FONT_GB12=ON` | 编译并写入 12 点阵中文字体 | `img/font12.c` | `/font/GB12.bin` |
| `-DWRITE_FONT_GB16=ON` | 预留 16 点阵中文字体开关 | `img/font16.c` | `/font/GB16.bin` |

当前你已经有的是：

```text
img/font12.c
```

以后如果增加 `img/font16.c`，再接 `WRITE_FONT_GB16`。

### 4.1 烧录 GB12 中文字体

CLion CMake options：

```text
-DWRITE_FONT_GB12=ON
```

然后在 `w25q80_init_fs();` 之后临时调用：

```c
#if WRITE_FONT_GB12
lfs_write_GB12();
#endif
```

示例位置：

```c
w25q80_init_fs(); // 初始化文件系统

#if WRITE_FONT_GB12
lfs_write_GB12();
#endif
```

写入目标文件：

```text
/font/GB12.bin
```

`lfs_write_GB12()` 会直接遍历 `img/font12.c` 里的：

```c
const typFNT_GB12 tfont12[] = { ... };
```

不再使用 `font12_1/font12_2/...` 分批宏。

### 4.2 关闭 GB12 字体烧录

烧录完成后，CLion CMake options 改回：

```text
-DWRITE_FONT_GB12=OFF
```

并注释掉临时调用，或者保留宏保护：

```c
#if WRITE_FONT_GB12
lfs_write_GB12();
#endif
```

只要宏是 OFF，就不会编译 `img/font12.c`，也不会写入。

### 4.3 ASCII 和数码管字体

以下函数目前在 `Font.c` 中始终存在于 `FLASH_WRITE` 保护内：

```c
lfs_write_asc_1206(); // 写 /font/ascii_1206.bin
lfs_write_asc_1608(); // 写 /font/ascii_1608.bin
lfs_write_sz_32();    // 写 /font/sz_32.bin
```

如果需要重新烧录这些基础字体，可以临时在 `w25q80_init_fs();` 后调用：

```c
lfs_write_asc_1206();
lfs_write_asc_1608();
lfs_write_sz_32();
```

写完后建议注释掉，避免每次开机重复写 Flash。

## 5. 常用烧录组合

### 5.1 只烧录 12x12 图片

CLion CMake options：

```text
-DWRITE_IMAGE_GROUP=12x12 -DWRITE_FONT_GB12=OFF -DWRITE_FONT_GB16=OFF
```

`main.c` 保持调用：

```c
write_images();
```

### 5.2 只烧录 GB12 字体

CLion CMake options：

```text
-DWRITE_IMAGE_GROUP=none -DWRITE_FONT_GB12=ON
```

`main.c` 临时调用：

```c
#if WRITE_FONT_GB12
lfs_write_GB12();
#endif
```

### 5.3 烧录 12x12 图片 + GB12 字体

CLion CMake options：

```text
-DWRITE_IMAGE_GROUP=12x12 -DWRITE_FONT_GB12=ON
```

`main.c`：

```c
w25q80_init_fs();
write_images();

#if WRITE_FONT_GB12
lfs_write_GB12();
#endif
```

### 5.4 正常运行固件

资源已经烧进 W25Q80 后，正常固件建议：

```text
-DWRITE_IMAGE_GROUP=none -DWRITE_FONT_GB12=OFF -DWRITE_FONT_GB16=OFF
```

并避免主动调用字体写入函数。

## 6. 注意事项

1. **每次只开需要烧录的资源组**  
   STM32F103C8T6 内部 Flash 只有 64KB。图片、字体数组只是临时搬运数据用，开太多容易导致固件超 Flash。

2. **烧完就关宏**  
   W25Q80 已经保存资源后，正常固件不需要再携带这些数组。

3. **图片建议分组烧录**  
   例如先烧 `12x12`，再烧 `24x24`，再烧 `24x48`，不要一次塞太多。

4. **字体当前只需要 GB12**  
   目前 `img/font12.c` 已存在，`font16.c` 以后需要时再添加。

5. **如文件系统被格式化，需要重新烧录资源**  
   如果执行了 littlefs format，`/images`、`/font` 下的资源都会丢失，需要重新按本文流程烧录。

6. **确认串口日志**  
   当前代码会通过 BLE/UART 打印部分文件系统信息，例如已用 block、目录列表等。烧录后可通过日志确认资源是否存在。
