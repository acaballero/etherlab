//
// Created by Angel Dust on 16/06/2021.
//

#ifndef TRX_FRONTEND_STM32F4XX_HAL_DAC_EX_CUSTOM_H
#define TRX_FRONTEND_STM32F4XX_HAL_DAC_EX_CUSTOM_H

#include "stm32f4xx.h"

#define DAC_CHANNEL_12D ((uint32_t)0x00000011)
void DAC_DMAConvCpltCh1(DMA_HandleTypeDef *hdma);
void DAC_DMAHalfConvCpltCh1(DMA_HandleTypeDef *hdma);
void DAC_DMAErrorCh1(DMA_HandleTypeDef *hdma);
HAL_StatusTypeDef HAL_DAC_Start_DualDMA(DAC_HandleTypeDef *hdac, uint32_t Channel, uint32_t *pData, uint32_t Length, uint32_t Alignment);

#endif //TRX_FRONTEND_STM32F4XX_HAL_DAC_EX_CUSTOM_H
