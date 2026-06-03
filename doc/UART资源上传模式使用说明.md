# UART1 调试串口说明

资源上传功能已经取消。外部 W25Q80 资源现在通过 STM32CubeProgrammer external loader 烧录，固件内不再保留 SecureCRT/XCOM/YMODEM/自定义上传协议。

USART1 仍保留为普通调试串口：

```c
#define ENABLE_UART1_DEBUG          1
```

对应初始化在 `User/System/main.c`：

```c
#if ENABLE_UART1_DEBUG
  uart1_init(115200); //USART1 普通调试串口
#endif
```

可用接口在 `User/Hardeware/Uart/uart.h`：

```c
void uart1_init(uint32_t bound);
int uart1_read_byte(uint8_t *ch, uint32_t timeout_ms);
int uart1_read(uint8_t *buf, uint32_t len, uint32_t timeout_ms);
void uart1_send(const uint8_t *buf, uint32_t len);
void uart1_printf(char *fmt, ...);
```

USART1 接收仍使用 512B 环形缓冲，适合普通调试命令或交互输入；只是资源上传协议和上传任务已移除。

注意：重新烧录 STM32 内部固件不会清空外部 W25Q80 中已有的 littlefs 资源。
