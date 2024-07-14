//
// Created by Angel Dust on 20/05/2021.
//

#ifndef TRX_FRONTEND_CLOCKS_H
#define TRX_FRONTEND_CLOCKS_H

#include <stdio.h>

#define APB1_PRESCALER 4
#define APB2_PRESCALER 2

#ifdef __cplusplus
extern "C" {
#endif

void HAL_MspInit(void);
void delay_us(uint32_t us);

#ifdef __cplusplus
}
#endif

void SystemClock_Config(void);

#endif //TRX_FRONTEND_CLOCKS_H
