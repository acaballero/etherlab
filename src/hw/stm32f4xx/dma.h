//
// Created by Angel Dust on 31/10/2019.
//

#ifndef TRX_FRONTEND_DMA_H
#define TRX_FRONTEND_DMA_H

#include <stm32f4xx.h>

#ifdef __cplusplus
extern "C" {
#endif


extern DMA_HandleTypeDef hdma_adc1;
//extern DMA_HandleTypeDef hdma_adc2;
extern DMA_HandleTypeDef hdma_spi2_tx;
extern DMA_HandleTypeDef hdma_dac_ch1;

extern DMA_HandleTypeDef hdma_sdio_rx;
extern DMA_HandleTypeDef hdma_sdio_tx;

extern bool firstDMAHalfSent;

void DMA1_Stream4_IRQHandler(void);
void DMA1_Stream5_IRQHandler(void);
void DMA2_Stream0_IRQHandler(void);
void DMA2_Stream3_IRQHandler(void);
void DMA2_Stream6_IRQHandler(void);

void HAL_SPI_TxHalfCpltCallback(SPI_HandleTypeDef *);

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *);

void MX_DMA_Init(void);

#ifdef __cplusplus
}
#endif

#endif //TRX_FRONTEND_DMA_H
