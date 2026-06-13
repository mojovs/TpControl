# font.txt 转 C 字库数组工具

`font_txt_to_c_array.py` 用来把 `font.txt` 中的 12x12 点阵字模转换成项目里 `typFNT_GB12` 使用的 C 数组格式。

## 输入格式

脚本会读取类似下面格式的行：

```c
0x04,0x06,...,"孵",0
```

要求：

- 每个字模一行；
- 行首必须是 `0x`；
- 每个字模必须有 24 个字节；
- 字符写在英文双引号中，例如 `"温"`；
- 最后的数字索引会被解析校验，但当前输出不会使用它；
- 如果同一个字出现多次，只保留第一次出现的字模，后面的重复字会自动跳过。

## 常用命令

### 1. 在项目根目录运行

```powershell
python img/font_txt_to_c_array.py img/font.txt -o img/tfont12.c
```

### 2. 进入 `img` 目录后运行

```powershell
cd img
python font_txt_to_c_array.py font.txt -o tfont12.c
```

### 3. 只打印到终端，不生成文件

```powershell
python img/font_txt_to_c_array.py img/font.txt
```

### 4. 指定数组名

默认输出数组名是 `tfont12`：

```c
const typFNT_GB12 tfont12[] = {
    "温",0x00,...,
};
```

如果要改数组名，用 `--array-name`：

```powershell
python img/font_txt_to_c_array.py img/font.txt -o img/my_font.c --array-name my_font
```

## 去重规则

脚本会检查重复字符，例如 `font.txt` 里如果出现两个 `"温"`：

- 第一次出现的 `"温"` 会被保留；
- 后面重复的 `"温"` 会被跳过；
- 终端会输出类似下面的提示：

```text
warning: line 123: duplicate character '温' skipped
```

这样生成的 C 数组里不会出现重复字模。

## 输出文件怎么用

生成 `img/tfont12.c` 后，打开该文件，把里面的数组内容复制到 `User/Hardeware/Lcd/Font.c` 中对应的中文 12x12 字库数组位置。

> 注意：脚本只生成数组本体，不会自动修改 `Font.c`，避免误覆盖现有字库。

## 报错说明

- `invalid glyph line`：某一行格式不符合 `0x...,"字",索引`。
- `expected 24 bytes`：该字模不是 12x12 点阵需要的 24 字节。

根据报错中的 `line 行号` 回到 `font.txt` 修改对应行即可。
