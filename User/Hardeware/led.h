//
// Created by meng on 2024/5/8.
//

#ifndef SCOPE_LED_H
#define SCOPE_LED_H

#include "tim.h"
#include "cmsis_os2.h"

void led_init();
void led_on();
void led_off();
#define LED1 PAout(9)
/**
 * led闪烁
 * @param times 闪烁的次数
 * @param delay 每次闪烁延迟时间，ms
 */
void led_shine(u16 times, u16 delay);
void led_set_compare(u32 comp);
void led_breath(u8 level, u8 speed, u16 cnt);
void light_gradient(u16 from, u16 to, u16 sec);
void light_set_compare(u32 comp);
void light_very_high(u32 sec);
void light_high(u32 sec);
void light_medium(u32 sec);
void light_low(u32 sec);
void light_breath(u8 level, u8 speed, u16 cnt);
#endif //SCOPE_LED_H