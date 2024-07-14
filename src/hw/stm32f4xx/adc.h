//
// Created by Angel Dust on 31/10/2019.
//

#ifndef TRX_FRONTEND_ADC_H
#define TRX_FRONTEND_ADC_H

#include "stm32f4xx.h"

#define ADC_MAX_SAMPLE_RATE (1200000UL)
#define MAX_ADC_VALUE 0x0FFF // 12 bits resolution
#define DUAL_ADC_CONV true // Dual simulateous ADCs or single ADC (alternating)

#ifdef __cplusplus
extern "C" {
#endif

extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;
extern ADC_HandleTypeDef hadc3;

extern uint8_t hadc1_mode;
extern uint8_t hadc2_mode;

extern bool adc_dma_started;

void HAL_ADC_MspInit(ADC_HandleTypeDef *);
void HAL_ADC_MspDeInit(ADC_HandleTypeDef *);
void MX_ADC1_Init(void);
void MX_ADC2_Init(void);
void MX_ADC3_Init(void);
void Config_ADC_DMA(void);
int GetADCValue(ADC_HandleTypeDef *hadc, uint32_t Channel, int count);
void ADC_DMA_Start(ADC_HandleTypeDef *hadc1);
void ADC_DMA_Stop(ADC_HandleTypeDef *hadc1);

#ifdef __cplusplus
}
#endif

void setup_adcs();

#endif //TRX_FRONTEND_ADC_H
