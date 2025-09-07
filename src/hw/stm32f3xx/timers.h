//
// Created by Angel Dust on 01/11/2019.
//

#ifndef TRX_FRONTEND_TIMERS_H
#define TRX_FRONTEND_TIMERS_H

#include "hw/stm32.h"

#ifdef __cplusplus
extern "C" {
#endif

extern TIM_HandleTypeDef htim3;  // blink led
extern TIM_HandleTypeDef htim4;  // DMA for ADC trigger
extern TIM_HandleTypeDef htim6;  // DMA for DAC trigger
extern TIM_HandleTypeDef htim15; // External storage task (sd card) trigger
void HAL_TIM_OC_MspInit(TIM_HandleTypeDef *htim_oc);
void HAL_TIM_OC_MspDeInit(TIM_HandleTypeDef *htim_oc);
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *);
void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *);
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *);
void MX_TIM3_Init(void);
void MX_TIM4_Init(void);
void MX_TIM6_Init(void);
void MX_TIM15_Init(void);

void update_timer(TIM_TypeDef *timer, uint32_t period, uint32_t prescaler);
void set_timer_sample_rate(TIM_TypeDef *timer, uint32_t clk_freq, uint32_t hz, uint32_t factor = 1);
void setup_timers();

#ifdef __cplusplus
}
#endif

#endif // TRX_FRONTEND_TIMERS_H
