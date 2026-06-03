//
// Created by Meng on 2025/11/21.
//

#ifndef KEY_H_H
#define KEY_H_H
#include "sys.h"
void key_init(void);
void ec11_spin(void);
/**
 * 旋转编码器 增减数值
 * @param val 要被加减的数值
 * @param max_min 最大加减值
 * @param mode 0 减，1 加
 */
void ec11_add_redcut(u8* val, u16 max_min, u8 mode);

void ec11_spin(void);

#endif //KEY_H_H

