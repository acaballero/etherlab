//
// Created by Angel Dust on 31/10/2019.
//
#include "hw/stm32.h"
#include "hw/hw_config.h"
#include "ui/view.h"
#include "hw/stm32f4xx/timers.h"
#include "ui/lcd.h"

ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc2;
ADC_HandleTypeDef hadc3;
ADC_HandleTypeDef hadc4;
uint8_t hadc1_mode = 0;
uint8_t hadc2_mode = 0;

bool adc_dma_started;

static uint32_t HAL_RCC_ADC12_CLK_ENABLED = 0;

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
static void ADC_Error_Handler(void) {
    /* USER CODE BEGIN ADC_Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */

#if LCD_ENABLED
    lcd.print("Fatal Error");
#endif
    /* USER CODE END ADC_Error_Handler_Debug */
}


/**
* @brief ADC MSP Initialization
* This function configures the hardware resources used in this example
* @param hadc: ADC handle pointer
* @retval None
*/
void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (hadc->Instance == ADC1) {
        /* USER CODE BEGIN ADC1_MspInit 0 */

        /* USER CODE END ADC1_MspInit 0 */
        /* Peripheral clock enable */
        HAL_RCC_ADC12_CLK_ENABLED++;
        if (HAL_RCC_ADC12_CLK_ENABLED == 1) {
            __HAL_RCC_ADC12_CLK_ENABLE();
        }


        __HAL_RCC_GPIOA_CLK_ENABLE();
        /**ADC1 GPIO Configuration
        PA0     ------> ADC1_IN1
        PA1     ------> Keyboad interrupt pin (not in analog mode)
        PA2     ------> ADC1_IN3
        PA3     ------> ADC1_IN4
        */
        GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_2 | GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* ADC1 DMA Init */
        /* ADC1 Init */

        hdma_adc1.Instance = DMA1_Channel1;
        hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;

#ifndef DMA_PALIGN
#error DMA_PALIGN must be declared first
#endif

        hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD; //DMA_PALIGN;
        hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_WORD; //DMA_MALIGN;

        hdma_adc1.Init.Mode = DMA_CIRCULAR;
        hdma_adc1.Init.Priority = DMA_PRIORITY_HIGH;
        if (HAL_DMA_Init(&hdma_adc1) != HAL_OK) {
            ADC_Error_Handler();
        }

        __HAL_LINKDMA(hadc, DMA_Handle, hdma_adc1);


        /* USER CODE BEGIN ADC1_MspInit 1 */

        /* USER CODE END ADC1_MspInit 1 */
    } else if (hadc->Instance == ADC2) {
        /* USER CODE BEGIN ADC2_MspInit 0 */

        /* USER CODE END ADC2_MspInit 0 */
        /* Peripheral clock enable */
        HAL_RCC_ADC12_CLK_ENABLED++;
        if (HAL_RCC_ADC12_CLK_ENABLED == 1) {
            __HAL_RCC_ADC12_CLK_ENABLE();
        }

        __HAL_RCC_GPIOA_CLK_ENABLE();
        /**ADC2 GPIO Configuration
        PA4     ------> ADC2_IN1
        */
        GPIO_InitStruct.Pin = GPIO_PIN_4;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* USER CODE BEGIN ADC2_MspInit 1 */

        /* USER CODE END ADC2_MspInit 1 */
    } else if (hadc->Instance == ADC3) {

        __HAL_RCC_ADC34_CLK_ENABLE();

        /**ADC3 GPIO Configuration
        PB13     ------> ADC3_IN5
        */
        GPIO_InitStruct.Pin = GPIO_PIN_13;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    } else if (hadc->Instance == ADC4) {

        __HAL_RCC_ADC34_CLK_ENABLE();

        /**ADC4 GPIO Configuration
        PB12     ------> ADC4_IN3
        */
        GPIO_InitStruct.Pin = GPIO_PIN_14;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    }
}


// Configure ACD1-CH3 and ADC2-CH1 in DMA DUAL SIMULTANEOUS MODE
void Config_ADC_DMA() {

    ADC_MultiModeTypeDef multimode = {0};
    ADC_ChannelConfTypeDef sConfig = {0};

    if (hadc1_mode !=
        1) { // Prevent reinitializing this ADC in the same mode (we're using it in both DMA and syncronous mode, using MX_ADC1_Init())

        hadc1_mode = 1;
        /* USER CODE BEGIN ADC1_Init 0 */

        /* USER CODE END ADC1_Init 0 */


        /* USER CODE BEGIN ADC1_Init 1 */

        /* USER CODE END ADC1_Init 1 */
        /** Common config
        */
        hadc1.Instance = ADC1;
        hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
        hadc1.Init.Resolution = ADC_RESOLUTION_12B;
        hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
        hadc1.Init.ContinuousConvMode = DISABLE;
        hadc1.Init.DiscontinuousConvMode = DISABLE;
        hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
        hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T4_TRGO;
        hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
        hadc1.Init.NbrOfConversion = 1; // 1 channel
        hadc1.Init.NbrOfDiscConversion = 0;
        hadc1.Init.DMAContinuousRequests = ENABLE; // ENABLED: Restart adquiring to the start of the buffer when the buffer is full
        hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
        hadc1.Init.LowPowerAutoWait = DISABLE;
        hadc1.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
        if (HAL_ADC_Init(&hadc1) != HAL_OK) {
            ADC_Error_Handler();
        }
        /** Configure the ADC multi-mode
        */
        multimode.Mode = ADC_DUALMODE_REGSIMULT;
        multimode.DMAAccessMode = ADC_DMAACCESSMODE_12_10_BITS;
        multimode.TwoSamplingDelay = ADC_TWOSAMPLINGDELAY_1CYCLE;
        if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK) {
            ADC_Error_Handler();
        }
        /** Configure Regular Channel
        */
        sConfig.Channel = ADC_CHANNEL_4;
        sConfig.Rank = ADC_REGULAR_RANK_1;
        sConfig.SingleDiff = ADC_SINGLE_ENDED;

        // The total conversion time is (sampling time + 12)*number of channels converted
        // So, for example, the conversion time for 2 channels with 7 cycles of sample time is 38 cycles
        // At 36Mhz ADC clock (a div2 prescaler from the 72Mhz system clock), the rate is 36/38 = 0.9Mhz
        // The timer used to trigger the conversions should not exceed this rate or ADC overruns will occur
        sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
        sConfig.OffsetNumber = ADC_OFFSET_1;
        sConfig.Offset = 2 << 10;
        if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
            ADC_Error_Handler();
        }
    }

    if (hadc2_mode != 1) {

        hadc2_mode = 1;
        // ADC2 as slave

        hadc2.Instance = ADC2;
        hadc2.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV1;
        hadc2.Init.Resolution = ADC_RESOLUTION_12B;
        hadc2.Init.ScanConvMode = ADC_SCAN_DISABLE;
        hadc2.Init.ContinuousConvMode = DISABLE;
        hadc2.Init.DiscontinuousConvMode = DISABLE;
        hadc2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
        hadc2.Init.NbrOfConversion = 1;
        hadc2.Init.DMAContinuousRequests = ENABLE;
        hadc2.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
        hadc2.Init.LowPowerAutoWait = DISABLE;
        hadc2.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
        if (HAL_ADC_Init(&hadc2) != HAL_OK) {
            ADC_Error_Handler();
        }
        /** Configure Regular Channel
        */
        sConfig.Channel = ADC_CHANNEL_1;
        sConfig.Rank = ADC_REGULAR_RANK_1;
        sConfig.SingleDiff = ADC_SINGLE_ENDED;
        sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
        sConfig.OffsetNumber = ADC_OFFSET_1;
        sConfig.Offset = 2 << 10;
        if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK) {
            ADC_Error_Handler();
        }

    }

}


/**
* @brief ADC MSP De-Initialization
* This function freeze the hardware resources used in this example
* @param hadc: ADC handle pointer
* @retval None
*/
void HAL_ADC_MspDeInit(ADC_HandleTypeDef *hadc) {

    if (hadc->Instance == ADC1) {
        /* USER CODE BEGIN ADC1_MspDeInit 0 */

        /* USER CODE END ADC1_MspDeInit 0 */
        /* Peripheral clock disable */
        HAL_RCC_ADC12_CLK_ENABLED--;
        __HAL_RCC_ADC12_CLK_DISABLE();

        /**ADC1 GPIO Configuration
        PA0     ------> ADC1_IN1

        PA2     ------> ADC1_IN3
        PA3     ------> ADC1_IN4


        */
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_0 | GPIO_PIN_2 | GPIO_PIN_3);

        /* ADC1 DMA DeInit */
        HAL_DMA_DeInit(hadc->DMA_Handle);

        /* USER CODE BEGIN ADC1_MspDeInit 1 */

        /* USER CODE END ADC1_MspDeInit 1 */
    } else if (hadc->Instance == ADC2) {
        /* USER CODE BEGIN ADC2_MspDeInit 0 */

        /* USER CODE END ADC2_MspDeInit 0 */
        /* Peripheral clock disable */
        HAL_RCC_ADC12_CLK_ENABLED--;
        if (HAL_RCC_ADC12_CLK_ENABLED == 0) {
            __HAL_RCC_ADC12_CLK_DISABLE();
        }

        /**ADC2 GPIO Configuration
        PA4     ------> ADC2_IN1
        */
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_4);

        /* USER CODE BEGIN ADC2_MspDeInit 1 */

        /* USER CODE END ADC2_MspDeInit 1 */
    } else if (hadc->Instance == ADC4) {


        /**ADC2 GPIO Configuration
        PB12     ------> ADC4_IN3
        */
        __HAL_RCC_ADC34_CLK_DISABLE();

        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_12);

    }
}

void MX_ADC1_Init(void) {

    /* USER CODE BEGIN ADC1_Init 0 */

    /* USER CODE END ADC1_Init 0 */

    hadc1_mode = 2;

    ADC_MultiModeTypeDef multimode = {0};
    ADC_ChannelConfTypeDef sConfig = {0};

    /* USER CODE BEGIN ADC1_Init 1 */

    /* USER CODE END ADC1_Init 1 */
    /** Common config
    */
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.NbrOfDiscConversion = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc1.Init.LowPowerAutoWait = DISABLE;
    hadc1.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) {
        ADC_Error_Handler();
    }
    /** Configure the ADC multi-mode
    */
    multimode.Mode = ADC_MODE_INDEPENDENT;
    if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK) {
        ADC_Error_Handler();
    }
    /** Configure Regular Channel
    */
    sConfig.Channel = ADC_CHANNEL_1;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.SamplingTime = ADC_SAMPLETIME_19CYCLES_5;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        ADC_Error_Handler();
    }



    /* USER CODE BEGIN ADC1_Init 2 */

    /* USER CODE END ADC1_Init 2 */

}



/**
  * @brief ADC2 Initialization Function
  * @param None
  * @retval None
  */
void MX_ADC2_Init(void) {

    /* USER CODE BEGIN ADC2_Init 0 */

    /* USER CODE END ADC2_Init 0 */

    hadc2_mode = 2;

    ADC_ChannelConfTypeDef sConfig = {0};

    /* USER CODE BEGIN ADC2_Init 1 */

    /* USER CODE END ADC2_Init 1 */
    /** Common config
    */
    hadc2.Instance = ADC2;
    hadc2.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
    hadc2.Init.Resolution = ADC_RESOLUTION_12B;
    hadc2.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc2.Init.ContinuousConvMode = DISABLE;
    hadc2.Init.DiscontinuousConvMode = DISABLE;
    hadc2.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc2.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc2.Init.NbrOfConversion = 1;
    hadc2.Init.DMAContinuousRequests = DISABLE;
    hadc2.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc2.Init.LowPowerAutoWait = DISABLE;
    hadc2.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
    if (HAL_ADC_Init(&hadc2) != HAL_OK) {
        ADC_Error_Handler();
    }
    /** Configure Regular Channel
    */
    sConfig.Channel = ADC_CHANNEL_1;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;
    if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK) {
        ADC_Error_Handler();
    }
    /* USER CODE BEGIN ADC2_Init 2 */

    /* USER CODE END ADC2_Init 2 */

}

void MX_ADC3_Init(void) {

    ADC_MultiModeTypeDef multimode = {0};
    ADC_ChannelConfTypeDef sConfig = {0};

    /* USER CODE BEGIN ADC1_Init 1 */

    /* USER CODE END ADC1_Init 1 */
    /** Common config
    */
    hadc3.Instance = ADC3;
    hadc3.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
    hadc3.Init.Resolution = ADC_RESOLUTION_12B;
    hadc3.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc3.Init.ContinuousConvMode = DISABLE;
    hadc3.Init.DiscontinuousConvMode = DISABLE;
    hadc3.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc3.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc3.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc3.Init.NbrOfConversion = 1;
    hadc3.Init.NbrOfDiscConversion = 1;
    hadc3.Init.DMAContinuousRequests = DISABLE;
    hadc3.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc3.Init.LowPowerAutoWait = DISABLE;
    hadc3.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
    if (HAL_ADC_Init(&hadc3) != HAL_OK) {
        ADC_Error_Handler();
    }
    /** Configure the ADC multi-mode
    */
    multimode.Mode = ADC_MODE_INDEPENDENT;
    if (HAL_ADCEx_MultiModeConfigChannel(&hadc3, &multimode) != HAL_OK) {
        ADC_Error_Handler();
    }
    /** Configure Regular Channel
    */
    sConfig.Channel = ADC_CHANNEL_5;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.SamplingTime = ADC_SAMPLETIME_19CYCLES_5;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;
    if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK) {
        ADC_Error_Handler();
    }


}

void MX_ADC4_Init(void) {

    ADC_MultiModeTypeDef multimode = {0};
    ADC_ChannelConfTypeDef sConfig = {0};

    /* USER CODE BEGIN ADC1_Init 1 */

    /* USER CODE END ADC1_Init 1 */
    /** Common config
    */
    hadc4.Instance = ADC4;
    hadc4.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
    hadc4.Init.Resolution = ADC_RESOLUTION_12B;
    hadc4.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc4.Init.ContinuousConvMode = DISABLE;
    hadc4.Init.DiscontinuousConvMode = DISABLE;
    hadc4.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc4.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc4.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc4.Init.NbrOfConversion = 1;
    hadc4.Init.NbrOfDiscConversion = 1;
    hadc4.Init.DMAContinuousRequests = DISABLE;
    hadc4.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc4.Init.LowPowerAutoWait = DISABLE;
    hadc4.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
    if (HAL_ADC_Init(&hadc4) != HAL_OK) {
        ADC_Error_Handler();
    }
    /** Configure the ADC multi-mode
    */
    multimode.Mode = ADC_MODE_INDEPENDENT;
    if (HAL_ADCEx_MultiModeConfigChannel(&hadc4, &multimode) != HAL_OK) {
        ADC_Error_Handler();
    }
    /** Configure Regular Channel
    */
    sConfig.Channel = ADC_CHANNEL_3;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.SamplingTime = ADC_SAMPLETIME_19CYCLES_5;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;
    if (HAL_ADC_ConfigChannel(&hadc4, &sConfig) != HAL_OK) {
        ADC_Error_Handler();
    }


}


int GetADCValue(ADC_HandleTypeDef *hadc, uint32_t Channel, int count) {

    // If we are reinitializing the same ADC between single shot and DMA mode we need to initialize for single shot here
    // TODO: Check if it's already in single shot mode to avoid reinitializing it in the same mode

    if (hadc == &hadc1) MX_ADC1_Init();
    else if (hadc == &hadc2) MX_ADC2_Init();

    int val = 0, v = 0;
    HAL_StatusTypeDef err = HAL_OK;
    ADC_ChannelConfTypeDef sConfig;
    sConfig.Channel = Channel;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    if (HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK) {
        ADC_Error_Handler();
    }
    HAL_ADC_Stop(hadc);
    for (int i = 0; i < count + 1 && err == HAL_OK; i++) {

        HAL_ADC_Start(hadc);
        //HAL_Delay(1); If a delay is needed, remember NOT to call this function inside an ISR (or adjust the ISR priorities so the tick counting has higher priority than the ISR)

        err = HAL_ADC_PollForConversion(hadc, 1000);

        if (err == HAL_OK && i > 0) { // trash de first reading as it's proved to be unreliable

            v = HAL_ADC_GetValue(hadc);
            val += v;
        }

        // If we are changing between DMA and direct conversion mode we should stop the ADC here. It seems that, otherwise, the DMA mode
        // won't fire interrupts (don't know why)
        HAL_ADC_Stop(hadc);
    }

    return val / count;
}

void ADC_DMA_Start(ADC_HandleTypeDef *hadc) {

    if (!adc_dma_started) {

        Config_ADC_DMA();

#if (DUAL_ADC_CONV)
        uint16_t capture_length = DSP_BLOCK * 2;

        // Start dual simultaneous conversions in ADCs 1&2
        HAL_ADCEx_MultiModeStart_DMA(hadc, (uint32_t *) adc_buff, capture_length);
        HAL_ADC_Start(&hadc2);

        HAL_TIM_Base_Start_IT(&htim4); // Start ACD DMA timer

#else

        uint16_t capture_length = DSP_BLOCK * 2 * 2;
        // Start single ADC sequenced conversion
        HAL_ADC_Start_DMA(hadc, (uint32_t *) &adc_buff, capture_length);
#endif
        adc_dma_started = true;
    }

}

void ADC_DMA_Stop(ADC_HandleTypeDef *hadc) {

    if (adc_dma_started) {

        HAL_TIM_Base_Stop_IT(&htim4); // Stop ACD DMA timer
#if DUAL_ADC_CONV
        // Stop dual simultaneous conversions in ADCs 1&2
        HAL_ADC_Stop(&hadc2);
        HAL_ADCEx_MultiModeStop_DMA(hadc);

#else
        // Stop single ADC sequenced conversion
        HAL_ADC_Stop_DMA(hadc);
#endif
        HAL_ADC_DeInit(&hadc1);
        HAL_ADC_DeInit(&hadc2);

        adc_dma_started = false;
        hadc1_mode=0;
        hadc2_mode=0;
    }
}

void setup_adcs() {

#if ENABLE_FFT
    MX_DMA_Init();
    Config_ADC_DMA();//MX_ADC1_Init();
    while (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK);
    while (HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED) != HAL_OK);
#endif

    //MX_ADC2_Init();
    MX_ADC3_Init();
    MX_ADC4_Init();

}
