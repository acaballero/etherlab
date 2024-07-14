//
// Created by Angel Dust on 23/03/2021.
//

#ifndef TRX_FRONTEND_DAC_H
#define TRX_FRONTEND_DAC_H

#include <stm32f4xx.h>

/**
 * In order to get the HAL dac implementation working, I've had to delete the __weak functinos in stm32fXXX_hal_dac.h files
 * Otherwise, they wouldn't get subsituted by the ones defined in stm32fXXX_hal_dac_ex.c.
 * This doesn't happen in stm32fXXX_adc.h / stm32fXXX_adc_ex.h though. Weird.
 */


#ifdef __cplusplus
extern "C" {
#endif

extern DAC_HandleTypeDef hdac1;

void MX_DAC_Init(void);
void HAL_DAC_MspInit(DAC_HandleTypeDef* hdac);
void HAL_DAC_MspDeInit(DAC_HandleTypeDef* hdac);
void DAC_DMA_Start(DAC_HandleTypeDef *hdac);
void DAC_DMA_Stop(DAC_HandleTypeDef *hdac);

#ifdef __cplusplus
}
#endif

#endif //TRX_FRONTEND_DAC_H
