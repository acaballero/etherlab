//
// Created by Angel Dust on 06/03/2021.
//

#ifndef TRX_FRONTEND_HANDLERS_H
#define TRX_FRONTEND_HANDLERS_H

#include "hw/stm32.h"

#ifdef __cplusplus
extern "C" {
#endif


void SysTick_Handler(void);
void EXTI15_10_IRQHandler(void);
void EXTI4_IRQHandler(void);
void EXTI9_5_IRQHandler(void);
void EXTI0_IRQHandler(void);
void Error_Handler(void);
void HardFault_Handler(void);


#ifdef __cplusplus
}
#endif

#endif //TRX_FRONTEND_HANDLERS_H
