//
// Created by Angel Dust on 04/04/2021.
//

#ifndef TRX_FRONTEND_DSP_H
#define TRX_FRONTEND_DSP_H

#include "hw/stm32.h"
#include "hw/hw_config.h"
#include "dsp_common.h"

#ifdef __cplusplus
extern "C" {
#endif

void dsp_init();
void dsp_set_real_time(bool);
uint8_t dsp_command(st_dspCommand command, void(*)(st_dspStatus *));
void dsp_loop();
inline void dsp_work();

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc);
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc);

void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef *hdac);
void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef *hdac);

// SD CARD FIFO processing handler
//void TIM1_BRK_TIM15_IRQHandler(void);
void TIM8_TRG_COM_TIM14_IRQHandler(void);

void dspSuccess();
void dspError(DSP_ERROR);

#ifdef __cplusplus
}
#endif

#endif //TRX_FRONTEND_DSP_H
