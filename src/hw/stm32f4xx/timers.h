//
// Created by Angel Dust on 01/11/2019.
//

#ifndef TRX_FRONTEND_TIMERS_H
#define TRX_FRONTEND_TIMERS_H

#include <stm32f4xx.h>

#ifdef __cplusplus
extern "C" {
#endif

extern TIM_HandleTypeDef htim3;  // blink led
extern TIM_HandleTypeDef htim2;  // DMA for ADC trigger
extern TIM_HandleTypeDef htim5;  // DMA for DAC trigger
extern TIM_HandleTypeDef htim6;  // USB task
extern TIM_HandleTypeDef htim7;  // FFT Acquisition
extern TIM_HandleTypeDef htim13; // Debouncer
extern TIM_HandleTypeDef htim14; // DSP tasks
void HAL_TIM_OC_MspInit(TIM_HandleTypeDef *htim_oc);
void HAL_TIM_OC_MspDeInit(TIM_HandleTypeDef *htim_oc);
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *);
void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *);
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *);
void MX_TIM3_Init(void);
void MX_TIM2_Init(void);
void MX_TIM5_Init(void);
void MX_TIM6_Init(void);
void MX_TIM7_Init(void);
void MX_TIM13_Init(void);
void MX_TIM14_Init(void);

void update_timer(TIM_TypeDef *timer, uint32_t period, uint32_t prescaler);
void set_timer_sample_rate(TIM_TypeDef *timer, uint32_t clk_freq, uint32_t hz, uint32_t factor = 1);
uint64_t get_adc_timer_frequency();
uint32_t get_timer_exact_freq(uint32_t factor, bool is16bits, uint32_t clk_freq, uint32_t hz);
void setup_timers();

/*
void TIM2_IRQHandler(void);
*/

#ifdef __cplusplus
}
#endif

#endif // TRX_FRONTEND_TIMERS_H
