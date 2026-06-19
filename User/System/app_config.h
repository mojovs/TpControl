//
// Created by Meng on 2026/6/2.
//

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/*
 * USART1 普通调试串口开关。
 *
 * 1: 保留 USART1 初始化、收发和 uart1_printf，用于串口调试辅助。
 * 0: 完全关闭 USART1 调试串口。
 */
#define ENABLE_UART1_DEBUG          0

/*
 * 强制格式化 littlefs 开关。
 *
 * 1: 每次启动都格式化文件系统（开发调试用，会清空所有数据）。
 * 0: 挂载失败时才自动格式化。
 */
#ifndef LFS_FORCE_FORMAT
#define LFS_FORCE_FORMAT            0
#endif

#endif //APP_CONFIG_H
