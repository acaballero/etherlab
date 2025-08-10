//
// Created by Angel Dust on 31/10/2019.
//

#include "dma.h"
#include "ui/lcd.h"

DMA_HandleTypeDef hdma_adc1;    // DMA handle for ADC (stream 0)
DMA_HandleTypeDef hdma_spi2_tx; // DMA handle for SPI (stream 4)
DMA_HandleTypeDef hdma_dac_ch1; // DMA handle for DAC (stream 5)
DMA_HandleTypeDef hdma_sdio_rx;
DMA_HandleTypeDef hdma_sdio_tx;

/**
 * @brief This function handles DMA2 stream3 global interrupt.
 */
void DMA2_Stream3_IRQHandler(void) {
    /* USER CODE BEGIN DMA2_Stream3_IRQn 0 */

    /* USER CODE END DMA2_Stream3_IRQn 0 */
    HAL_DMA_IRQHandler(&hdma_sdio_rx);
    /* USER CODE BEGIN DMA2_Stream3_IRQn 1 */

    /* USER CODE END DMA2_Stream3_IRQn 1 */
}

/**
 * @brief This function handles DMA2 stream6 global interrupt.
 */
void DMA2_Stream6_IRQHandler(void) {
    /* USER CODE BEGIN DMA2_Stream6_IRQn 0 */

    /* USER CODE END DMA2_Stream6_IRQn 0 */
    HAL_DMA_IRQHandler(&hdma_sdio_tx);
    /* USER CODE BEGIN DMA2_Stream6_IRQn 1 */

    /* USER CODE END DMA2_Stream6_IRQn 1 */
}

void DMA2_Stream0_IRQHandler(void) {
    /* USER CODE BEGIN DMA1_Channel1_IRQn 0 */

    /* USER CODE END DMA1_Channel1_IRQn 0 */
    HAL_DMA_IRQHandler(&hdma_adc1);
    /* USER CODE BEGIN DMA1_Channel1_IRQn 1 */

    /* USER CODE END DMA1_Channel1_IRQn 1 */
}

/**
 * @brief This function handles DMA1 stream 4 global interrupt.
 */
void DMA1_Stream4_IRQHandler(void) {
    /* USER CODE BEGIN DMA1_Channel3_IRQn 0 */

    /* USER CODE END DMA1_Channel3_IRQn 0 */
    HAL_DMA_IRQHandler(&hdma_spi2_tx);
    /* USER CODE BEGIN DMA1_Channel3_IRQn 1 */

    /* USER CODE END DMA1_Channel3_IRQn 1 */
}

/**
 * @brief This function handles DMA1 stream 5 global interrupt.
 */
void DMA1_Stream5_IRQHandler(void) {
    /* USER CODE BEGIN DMA2_Channel3_IRQn 0 */

    /* USER CODE END DMA2_Channel3_IRQn 0 */
    HAL_DMA_IRQHandler(&hdma_dac_ch1);
    /* USER CODE BEGIN DMA2_Channel3_IRQn 1 */

    /* USER CODE END DMA2_Channel3_IRQn 1 */
}

/**
 * Half transfer complete callback
 * @param hspi
 */
void HAL_SPI_TxHalfCpltCallback(SPI_HandleTypeDef *hspi) {
    lcd.DMATxHalfCpltCallback();
}

/**
 * Transfer complete callback
 * @param hspi
 */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
    lcd.DMATxCpltCallback();
}

void MX_DMA_Init(void) {

    /* DMA controller clock enable */
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();

    /* DMA interrupt init */
    /* DMA1_Stream4_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);
    /* DMA1_Stream5_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
    /* DMA2_Stream0_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
    /* DMA2_Stream3_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);
    /* DMA2_Stream6_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA2_Stream6_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream6_IRQn);
}
