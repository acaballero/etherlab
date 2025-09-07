//
// Created by Angel Dust on 23/03/2021.
//
#include "dac.h"
#include "hw/stm32f4xx/timers.h"
#include "main.h"
#include "handlers.h"
#include "stm32f4xx_hal_dac_ex.h"
#include "stm32f4xx_hal_dac_ex_custom.h"
#include "status.h"
#include "stm32f4xx_hal_def.h"

DAC_HandleTypeDef hdac1;

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void DAC_Error_Handler(void) {
    /* USER CODE BEGIN ADC_Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */

    /* USER CODE END ADC_Error_Handler_Debug */
}

/**
 * @brief DAC Initialization Function
 * @param None
 * @retval None
 */
void MX_DAC_Init(void) {

    /* USER CODE BEGIN DAC_Init 0 */

    /* USER CODE END DAC_Init 0 */

    DAC_ChannelConfTypeDef sConfig = {0};

    /* USER CODE BEGIN DAC_Init 1 */

    /* USER CODE END DAC_Init 1 */
    /** DAC Initialization
     */
    hdac1.Instance = DAC;

    if (HAL_DAC_Init(&hdac1) != HAL_OK) {
        Error_Handler();
    }
    /** DAC channel OUT1 config
     */
    sConfig.DAC_Trigger = DAC_TRIGGER_T5_TRGO; // TIM5 triggers the DAC
    sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;

    if (HAL_DAC_ConfigChannel(&hdac1, &sConfig, DAC_CHANNEL_1) != HAL_OK) {
        Error_Handler();
    }

    /** DAC channel OUT2 config
     */
    if (HAL_DAC_ConfigChannel(&hdac1, &sConfig, DAC_CHANNEL_2) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN DAC_Init 2 */

    /* USER CODE END DAC_Init 2 */
}

/**
 * @brief DAC MSP Initialization
 * This function configures the hardware resources used in this example
 * @param hdac: DAC handle pointer
 * @retval None
 */
void HAL_DAC_MspInit(DAC_HandleTypeDef *hdac) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (hdac->Instance == DAC) {
        /* USER CODE BEGIN DAC_MspInit 0 */

        /* USER CODE END DAC_MspInit 0 */
        /* Peripheral clock enable */
        __HAL_RCC_DAC_CLK_ENABLE();

        __HAL_RCC_GPIOA_CLK_ENABLE();
        /**DAC GPIO Configuration
        PA4     ------> DAC_OUT1
        PA5     ------> DAC_OUT2
        */
        GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_5;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* DAC DMA Init */
        /* DAC_CH1 Init */
        hdma_dac_ch1.Instance = DMA1_Stream5;
        hdma_dac_ch1.Init.Channel = DMA_CHANNEL_7;
        hdma_dac_ch1.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_dac_ch1.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_dac_ch1.Init.MemInc = DMA_MINC_ENABLE;
        hdma_dac_ch1.Init.Mode = DMA_CIRCULAR;
        hdma_dac_ch1.Init.Priority = DMA_PRIORITY_LOW;
        hdma_dac_ch1.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        // hdma_dac_ch1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
        // hdma_dac_ch1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
        hdma_dac_ch1.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD; // WORD if the DAC buffer is complex and I/Q samples are interleaved int16_t
        hdma_dac_ch1.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;

        if (HAL_DMA_Init(&hdma_dac_ch1) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(hdac, DMA_Handle1, hdma_dac_ch1);

        /* USER CODE BEGIN DAC_MspInit 1 */

        /* USER CODE END DAC_MspInit 1 */
    }
}

/**
 * @brief DAC MSP De-Initialization
 * This function freeze the hardware resources used in this example
 * @param hdac: DAC handle pointer
 * @retval None
 */
void HAL_DAC_MspDeInit(DAC_HandleTypeDef *hdac) {
    if (hdac->Instance == DAC) {
        /* USER CODE BEGIN DAC_MspDeInit 0 */

        /* USER CODE END DAC_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_DAC_CLK_DISABLE();

        /**DAC GPIO Configuration
        PA4     ------> DAC_OUT1
        PA5     ------> DAC_OUT2
        */
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_4 | GPIO_PIN_5);

        /* DAC DMA DeInit */
        HAL_DMA_DeInit(hdac->DMA_Handle1);
        /* USER CODE BEGIN DAC_MspDeInit 1 */

        /* USER CODE END DAC_MspDeInit 1 */
    }
}

volatile bool dac_dma_started = false;
void DAC_DMA_Start(DAC_HandleTypeDef *hdac) {

    if (!dac_dma_started) {
        // LOG("DAC_DMA_START\n");
        HAL_StatusTypeDef ret = HAL_TIM_Base_Start(&htim5); // Start DAC DMA timer

        if (ret == HAL_OK) {
            // We are using a complex buffer of int16_t, so the length of the buffer is DSP_BLOCK*2 to fill the whole dac_buff

            ret = HAL_DAC_Start_DualDMA(hdac, DAC_CHANNEL_12D, (uint32_t *)dac_buff, DSP_BLOCK * 2, DAC_ALIGN_12B_R);

            if (ret != HAL_OK) {
                // LOG("HAL_DAC_Start_DualDMA ERROR!!\n");
            } else {
                dac_dma_started = true;
            }
        } else {
            // LOG("HAL_TIM_Base_Start ERROR!!\n");
        }
    } else {
        // LOG("DAC_DMA_START: Did nothing\n");
    }
}

void DAC_DMA_Stop(DAC_HandleTypeDef *hdac) {
    if (dac_dma_started) {
        // LOG("DAC_DMA_STOP\n");
        HAL_TIM_Base_Stop(&htim5); // Stop DAC DMA timer
        HAL_DAC_Stop_DMA(hdac, DAC_CHANNEL_1);
        HAL_DAC_Stop_DMA(hdac, DAC_CHANNEL_2);
        dac_dma_started = false;
    } else {
        // LOG("DAC_DMA_STOP: Did nothing\n");
    }
}
