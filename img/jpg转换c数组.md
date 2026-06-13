# JPG 图标转 TFT 图模数组工具

`jpg_to_tft_c_array.py` 用来把当前目录下的 `.jpg/.jpeg` 图标转换成项目 TFT 驱动可直接写入 LCD 的 RGB565 图模数组。

每张 JPG 会生成一个单独的 `.c` 文件，并额外生成一个统一头文件，头文件默认使用目录名命名，例如：

```text
img/icon_24x24/egg.jpg      -> img/icon_24x24/egg_24x24.c
img/icon_24x24/printer.jpg  -> img/icon_24x24/printer_24x24.c
                            -> img/icon_24x24/icon_24x24.h
```

生成的 C 数组格式类似：

```c
const unsigned char gImage_egg_24x24[1152] = { /* 0X00,0X10,0X18,0X00,0X18,0X00,0X01,0X1B, */
0XFF,0XFF,...
};
```

其中数据格式是：

- RGB565；
- 每像素 2 字节；
- 低字节在前，高字节在后；
- 从左到右、从上到下扫描。

这个顺序匹配当前 `Gui_showimage_12x12()` / `Gui_showimage_24x24()` 中的读取方式。

## 准备环境

脚本需要 Python 的 Pillow 库读取 JPG。

如果没有安装，先执行：

```powershell
python -m pip install pillow
```

## 常用命令

### 1. 在某个 icon 目录里转换当前目录所有 JPG

例如转换 `img/icon_24x24`：

```powershell
cd img/icon_24x24
python ../jpg_to_tft_c_array.py
```

生成的 `.c` 文件和目录同名 `.h` 头文件会放在当前目录，也就是 `img/icon_24x24`。

### 2. 在项目根目录指定目录转换

```powershell
python img/jpg_to_tft_c_array.py img/icon_24x24
```

### 3. 转换 12x12 图标

```powershell
python img/jpg_to_tft_c_array.py img/icon_12x12
```

### 4. 转换 24x48 图标

```powershell
python img/jpg_to_tft_c_array.py img/icon_24x48
```

### 5. 输出到指定目录

```powershell
python img/jpg_to_tft_c_array.py img/icon_24x24 -o img/icon_24x24/out
```

### 6. 修改数组名前缀

默认数组名前缀是 `gImage_`，数组名也会自动加尺寸后缀。例如 `egg.jpg` 尺寸是 24x24 时，会生成：

```c
const unsigned char gImage_egg_24x24[1152] = { ... };
```

如果想改前缀：

```powershell
python img/jpg_to_tft_c_array.py img/icon_24x24 --prefix icon_
```

会生成：

```c
const unsigned char icon_egg_24x24[1152] = { ... };
```

### 7. 自动生成统一头文件

脚本默认会额外生成一个目录同名头文件，里面包含所有生成数组的 `extern` 声明，方便在 `Image.h` 或其他文件里统一引用。

例如：

- 输入目录是 `img/icon_12x12`，生成 `img/icon_12x12/icon_12x12.h`；
- 输入目录是 `img/icon_24x24`，生成 `img/icon_24x24/icon_24x24.h`；
- 输入目录是 `img/icon_24x48`，生成 `img/icon_24x48/icon_24x48.h`。

生成内容类似：

```c
#ifndef ICON_24X24_H
#define ICON_24X24_H

/* egg_24x24.c: 24x24, 1152 bytes */
extern const unsigned char gImage_egg_24x24[1152];

#endif /* ICON_24X24_H */
```

如果要改头文件名，用 `--header`：

```powershell
python img/jpg_to_tft_c_array.py img/icon_24x24 --header icon_24x24_images.h
```

如果不想生成头文件，可以加 `--no-header`：

```powershell
python img/jpg_to_tft_c_array.py img/icon_24x24 --no-header
```

## 生成后的使用方式

如果要把图模写入 W25Q80/littlefs，可以在 `User/Hardeware/Lcd/Image.c` 中包含对应 `.c` 文件，然后调用 `write_img()` 写入 flash。

示例：

```c
#include "../../../img/icon_24x24/icon_24x24.h"
#include "../../../img/icon_24x24/egg_24x24.c"

void write_img_egg(void)
{
    write_img("/images/egg.bin", gImage_egg_24x24, sizeof(gImage_egg_24x24));
}
```

显示时：

```c
Gui_showimage_24x24("/images/egg.bin", x, y);
```

12x12 图标使用：

```c
Gui_showimage_12x12("/images/hot.bin", x, y);
```

## 注意事项

- 脚本只处理指定目录下的 JPG，不会递归处理子目录；
- 每张图片生成一个独立 `.c` 文件，文件名会自动加尺寸后缀，例如 `egg_24x24.c`；
- C 数组名也会自动加尺寸后缀，例如 `gImage_egg_24x24`；
- 图片尺寸不会自动缩放，12x12/24x24/24x48 请提前把 JPG 调整到目标尺寸；
- 数组大小 = 宽度 × 高度 × 2，例如：
  - 12x12 = 288 字节；
  - 24x24 = 1152 字节；
  - 24x48 = 2304 字节。
