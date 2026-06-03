//
// Created by Meng on 2025/11/25.
//

#ifndef HUMIDITY_H
#define HUMIDITY_H
#include "sys.h"
void humidity_init(void);
void humidity_start(void);
u8  humidity_read(void);
void humidity_stop(void);

#endif //HUMIDITY_H

