//
// Created by Angel Dust on 23/03/2021.
//
#include "hw/stm32.h"
#include "ui/view.h"
#include "dac.h"
#include "hw/stm32f4xx/dma.h"
#include "hw/stm32f4xx/timers.h"
#include "main.h"
#include "stm32f3xx_hal_dac_ex.h"

DAC_HandleTypeDef hdac1;
OPAMP_HandleTypeDef hopamp4;

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
void MX_DAC_Init(void)
{

    /* USER CODE BEGIN DAC_Init 0 */

    /* USER CODE END DAC_Init 0 */

    DAC_ChannelConfTypeDef sConfig = {0};

    /* USER CODE BEGIN DAC_Init 1 */

    /* USER CODE END DAC_Init 1 */
    /** DAC Initialization
    */
    hdac1.Instance = DAC;
    if (HAL_DAC_Init(&hdac1) != HAL_OK)
    {
        Error_Handler();
    }
    /** DAC channel OUT1 config
    */
    sConfig.DAC_Trigger = DAC_TRIGGER_T6_TRGO;
    sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;

    if (HAL_DAC_ConfigChannel(&hdac1, &sConfig, DAC_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN DAC_Init 2 */

    /* USER CODE END DAC_Init 2 */

}



/**
  * @brief OPAMP4 Initialization Function
  * @param None
  * @retval None
  */
void MX_OPAMP4_Init(void)
{

    /* USER CODE BEGIN OPAMP4_Init 0 */

    /* USER CODE END OPAMP4_Init 0 */

    /* USER CODE BEGIN OPAMP4_Init 1 */

    /* USER CODE END OPAMP4_Init 1 */
    hopamp4.Instance = OPAMP4;
    hopamp4.Init.Mode = OPAMP_FOLLOWER_MODE;
    hopamp4.Init.NonInvertingInput = OPAMP_NONINVERTINGINPUT_IO2;
    hopamp4.Init.TimerControlledMuxmode = OPAMP_TIMERCONTROLLEDMUXMODE_DISABLE;
    hopamp4.Init.UserTrimming = OPAMP_TRIMMING_FACTORY;
    if (HAL_OPAMP_Init(&hopamp4) != HAL_OK)
    {
        Error_Handler();
    }

    /* USER CODE BEGIN OPAMP4_Init 2 */

    /* USER CODE END OPAMP4_Init 2 */

}



/**
* @brief DAC MSP Initialization
* This function configures the hardware resources used in this example
* @param hdac: DAC handle pointer
* @retval None
*/
void HAL_DAC_MspInit(DAC_HandleTypeDef* hdac)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if(hdac->Instance==DAC)
    {
        /* USER CODE BEGIN DAC_MspInit 0 */

        /* USER CODE END DAC_MspInit 0 */
        /* Peripheral clock enable */
        __HAL_RCC_DAC1_CLK_ENABLE();

        __HAL_RCC_GPIOA_CLK_ENABLE();
        /**DAC GPIO Configuration
        PA4     ------> DAC_OUT1
        */
        GPIO_InitStruct.Pin = GPIO_PIN_4;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* DAC DMA Init */
        /* DAC_CH1 Init */
        hdma_dac_ch1.Instance = DMA2_Channel3;
        hdma_dac_ch1.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_dac_ch1.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_dac_ch1.Init.MemInc = DMA_MINC_ENABLE;
        //hdma_dac_ch1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
        //hdma_dac_ch1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
        hdma_dac_ch1.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD; // WORD if the DAC buffer is complex and I/Q samples are interleaved int16_t
        hdma_dac_ch1.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
        hdma_dac_ch1.Init.Mode = DMA_CIRCULAR;
        hdma_dac_ch1.Init.Priority = DMA_PRIORITY_LOW;
        if (HAL_DMA_Init(&hdma_dac_ch1) != HAL_OK)
        {
            Error_Handler();
        }

        __HAL_LINKDMA(hdac,DMA_Handle1,hdma_dac_ch1);

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
void HAL_DAC_MspDeInit(DAC_HandleTypeDef* hdac)
{
    if(hdac->Instance==DAC)
    {
        /* USER CODE BEGIN DAC_MspDeInit 0 */

        /* USER CODE END DAC_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_DAC1_CLK_DISABLE();

        /**DAC GPIO Configuration
        PA4     ------> DAC_OUT1
        */
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_4);

        /* USER CODE BEGIN DAC_MspDeInit 1 */

        /* USER CODE END DAC_MspDeInit 1 */
    }

}


/**
* @brief OPAMP MSP Initialization
* This function configures the hardware resources used in this example
* @param hopamp: OPAMP handle pointer
* @retval None
*/
void HAL_OPAMP_MspInit(OPAMP_HandleTypeDef* hopamp)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if(hopamp->Instance==OPAMP4)
    {
        /* USER CODE BEGIN OPAMP4_MspInit 0 */

        /* USER CODE END OPAMP4_MspInit 0 */

        __HAL_RCC_GPIOB_CLK_ENABLE();
        /**OPAMP4 GPIO Configuration
        PB12     ------> OPAMP4_VOUT
        */
        GPIO_InitStruct.Pin = GPIO_PIN_12;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* USER CODE BEGIN OPAMP4_MspInit 1 */

        /* USER CODE END OPAMP4_MspInit 1 */
    }

}

/**
* @brief OPAMP MSP De-Initialization
* This function freeze the hardware resources used in this example
* @param hopamp: OPAMP handle pointer
* @retval None
*/
void HAL_OPAMP_MspDeInit(OPAMP_HandleTypeDef* hopamp)
{
    if(hopamp->Instance==OPAMP4)
    {
        /* USER CODE BEGIN OPAMP4_MspDeInit 0 */

        /* USER CODE END OPAMP4_MspDeInit 0 */

        /**OPAMP4 GPIO Configuration
        PB12     ------> OPAMP4_VOUT
        */
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_12);

        /* USER CODE BEGIN OPAMP4_MspDeInit 1 */

        /* USER CODE END OPAMP4_MspDeInit 1 */
    }

}


void DAC_DMA_Start(DAC_HandleTypeDef *hdac) {

    HAL_TIM_Base_Start(&htim6); // Start DAC DMA timer

    // We are using a complex buffer of int16_t, so the length of the buffer is DSP_BLOCK*2 to fill the whole dac_buff

    HAL_DAC_Start_DMA(hdac, DAC_CHANNEL_1, (uint32_t*)dac_buff, DSP_BLOCK*2, DAC_ALIGN_12B_R);

}

void DAC_DMA_Stop(DAC_HandleTypeDef *hdac) {

    HAL_TIM_Base_Stop(&htim6); // Start DAC DMA timer
    HAL_DAC_Stop_DMA(hdac,DAC_CHANNEL_1);
}