//
// Created by Angel Dust on 31/10/2019.
//
#include "adc.h"
#include "hw/hw_config.h"
#include "stm32f4xx_hal_adc.h"
#include "ui/lcd.h"
#include "status.h"

ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc2;
ADC_HandleTypeDef hadc3;

uint8_t hadc1_mode = 0;
uint8_t hadc2_mode = 0;

bool adc_dma_started;

static uint32_t HAL_RCC_ADC1_CLK_ENABLED = 0;
static uint32_t HAL_RCC_ADC2_CLK_ENABLED = 0;

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
        HAL_RCC_ADC1_CLK_ENABLED++;
        if (HAL_RCC_ADC1_CLK_ENABLED == 1) {
            __HAL_RCC_ADC1_CLK_ENABLE();
        }

        __HAL_RCC_GPIOC_CLK_ENABLE();
        /**ADC1 GPIO Configuration
        PC4     ------> ADC1_IN14
        */
        GPIO_InitStruct.Pin = GPIO_PIN_4;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        /* ADC1 DMA Init */
        /* ADC1 Init */

        hdma_adc1.Instance = DMA2_Stream0;
        hdma_adc1.Init.Channel = DMA_CHANNEL_0;
        hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;

#ifndef DMA_PALIGN
#error DMA_PALIGN must be declared first
#endif

        hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD; // DMA_PALIGN;
        hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;    // DMA_MALIGN;
        hdma_adc1.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
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
        HAL_RCC_ADC2_CLK_ENABLED++;
        if (HAL_RCC_ADC2_CLK_ENABLED == 1) {
            __HAL_RCC_ADC2_CLK_ENABLE();
        }

        __HAL_RCC_GPIOC_CLK_ENABLE();
        /**ADC2 GPIO Configuration
    PC5     ------> ADC2_IN15
    */
        GPIO_InitStruct.Pin = GPIO_PIN_5;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        /* USER CODE BEGIN ADC2_MspInit 1 */

        /* USER CODE END ADC2_MspInit 1 */
    } else if (hadc->Instance == ADC3) {

        __HAL_RCC_ADC3_CLK_ENABLE();

        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /**ADC3 GPIO Configuration
   PC0     ------> ADC3_IN10
   PC1     ------> ADC3_IN11
   PC2     ------> ADC3_IN12
   PA0/WKUP     ------> ADC3_IN0
   PA1     ------> ADC3_IN1
   PA2     ------> ADC3_IN2
   PA3     ------> ADC3_IN3
   */
        GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}

// Configure ACD1-CH3 and ADC2-CH1 in DMA DUAL SIMULTANEOUS MODE
void Config_ADC_DMA() {

    ADC_MultiModeTypeDef multimode = {0};
    ADC_ChannelConfTypeDef sConfig = {0};

    if (hadc1_mode != 1) { // Prevent reinitializing this ADC in the same mode (we're using it in both DMA and synchronous mode, using MX_ADC1_Init())

        hadc1_mode = 1;
        /* USER CODE BEGIN ADC1_Init 0 */

        /* USER CODE END ADC1_Init 0 */

        /* USER CODE BEGIN ADC1_Init 1 */

        /* USER CODE END ADC1_Init 1 */
        /** Common config
         */
        hadc1.Instance = ADC1;
        hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
        hadc1.Init.Resolution = ADC_RESOLUTION_12B;
        hadc1.Init.ScanConvMode = DISABLE;
        hadc1.Init.ContinuousConvMode = DISABLE;
        hadc1.Init.DiscontinuousConvMode = DISABLE;
        hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
        hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T2_TRGO; // ADC_EXTERNALTRIGCONV_T2_TRGO;
        hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
        hadc1.Init.NbrOfConversion = 1; // 1 channel
        hadc1.Init.NbrOfDiscConversion = 0;
        hadc1.Init.DMAContinuousRequests = ENABLE; // ENABLED: Restart adquiring to the start of the buffer when the buffer is full
        hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;

        if (HAL_ADC_Init(&hadc1) != HAL_OK) {
            ADC_Error_Handler();
        }
        /** Configure the ADC multi-mode
         */
        multimode.Mode = ADC_DUALMODE_REGSIMULT;
        multimode.DMAAccessMode = ADC_DMAACCESSMODE_2;
        multimode.TwoSamplingDelay = ADC_TWOSAMPLINGDELAY_5CYCLES;
        if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK) {
            ADC_Error_Handler();
        }
        /** Configure Regular Channel
         */
        sConfig.Channel = ADC_CHANNEL_14;
        sConfig.Rank = 1;
        // sConfig.SingleDiff = ADC_SINGLE_ENDED;

        // The total conversion time is (sampling time + 12)*number of channels converted
        // So, for example, the conversion time for 2 channels with 7 cycles of sample time is 38 cycles
        // At 36Mhz ADC clock (a div2 prescaler from the 72Mhz system clock), the rate is 36/38 = 0.9Mhz
        // The timer used to trigger the conversions should not exceed this rate or ADC overruns will occur
        sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
        // sConfig.OffsetNumber = 1;
        sConfig.Offset = 1000;
        if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
            ADC_Error_Handler();
        }
    }

    if (hadc2_mode != 1) {

        hadc2_mode = 1;
        // ADC2 as slave

        hadc2.Instance = ADC2;
        hadc2.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
        hadc2.Init.Resolution = ADC_RESOLUTION_12B;
        hadc2.Init.ScanConvMode = DISABLE;
        hadc2.Init.ContinuousConvMode = DISABLE;
        hadc2.Init.DiscontinuousConvMode = DISABLE;
        hadc2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
        hadc2.Init.NbrOfConversion = 1;
        hadc2.Init.DMAContinuousRequests = DISABLE;
        hadc2.Init.EOCSelection = ADC_EOC_SEQ_CONV;

        if (HAL_ADC_Init(&hadc2) != HAL_OK) {
            ADC_Error_Handler();
        }
        /** Configure Regular Channel
         */
        sConfig.Channel = ADC_CHANNEL_15;
        sConfig.Rank = 1;
        // sConfig.SingleDiff = ADC_SINGLE_ENDED;
        sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
        // sConfig.OffsetNumber = ADC_OFFSET_1;
        sConfig.Offset = 1000;
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
        HAL_RCC_ADC1_CLK_ENABLED--;
        __HAL_RCC_ADC1_CLK_DISABLE();

        /**ADC1 GPIO Configuration
         PC4     ------> ADC1_IN14
         */
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_4);

        /* ADC1 DMA DeInit */
        HAL_DMA_DeInit(hadc->DMA_Handle);

        /* USER CODE BEGIN ADC1_MspDeInit 1 */

        /* USER CODE END ADC1_MspDeInit 1 */
    } else if (hadc->Instance == ADC2) {
        /* USER CODE BEGIN ADC2_MspDeInit 0 */

        /* USER CODE END ADC2_MspDeInit 0 */
        /* Peripheral clock disable */
        HAL_RCC_ADC2_CLK_ENABLED--;
        if (HAL_RCC_ADC2_CLK_ENABLED == 0) {
            __HAL_RCC_ADC2_CLK_DISABLE();
        }

        /**ADC2 GPIO Configuration
        PC5     ------> ADC2_IN15
        */
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_5);

        /* USER CODE BEGIN ADC2_MspDeInit 1 */

        /* USER CODE END ADC2_MspDeInit 1 */
    } else if (hadc->Instance == ADC3) {
        /* USER CODE BEGIN ADC3_MspDeInit 0 */

        /* USER CODE END ADC3_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_ADC3_CLK_DISABLE();

        /**ADC3 GPIO Configuration
        PC0     ------> ADC3_IN10
        PC1     ------> ADC3_IN11
        PC2     ------> ADC3_IN12
        PA0/WKUP     ------> ADC3_IN0
        PA1     ------> ADC3_IN1
        PA2     ------> ADC3_IN2
        PA3     ------> ADC3_IN3
        */
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2);

        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);

        /* USER CODE BEGIN ADC3_MspDeInit 1 */

        /* USER CODE END ADC3_MspDeInit 1 */
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
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.NbrOfDiscConversion = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;

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
    sConfig.Channel = ADC_CHANNEL_14;
    sConfig.Rank = 1;

    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;

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
    hadc2.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc2.Init.Resolution = ADC_RESOLUTION_12B;
    hadc2.Init.ScanConvMode = DISABLE;
    hadc2.Init.ContinuousConvMode = DISABLE;
    hadc2.Init.DiscontinuousConvMode = DISABLE;
    hadc2.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc2.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc2.Init.NbrOfConversion = 1;
    hadc2.Init.DMAContinuousRequests = DISABLE;
    hadc2.Init.EOCSelection = ADC_EOC_SINGLE_CONV;

    if (HAL_ADC_Init(&hadc2) != HAL_OK) {
        ADC_Error_Handler();
    }
    /** Configure Regular Channel
     */
    sConfig.Channel = ADC_CHANNEL_15;
    sConfig.Rank = 1;
    // sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;

    sConfig.Offset = 0;
    if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK) {
        ADC_Error_Handler();
    }
    /* USER CODE BEGIN ADC2_Init 2 */

    /* USER CODE END ADC2_Init 2 */
}

void MX_ADC3_Init(void) {

    /* USER CODE BEGIN ADC3_Init 0 */

    /* USER CODE END ADC3_Init 0 */

    ADC_ChannelConfTypeDef sConfig = {0};

    /* USER CODE BEGIN ADC3_Init 1 */

    /* USER CODE END ADC3_Init 1 */
    /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
     */
    hadc3.Instance = ADC3;
    hadc3.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV8;
    hadc3.Init.Resolution = ADC_RESOLUTION_12B;
    hadc3.Init.ScanConvMode = DISABLE;
    hadc3.Init.ContinuousConvMode = DISABLE;
    hadc3.Init.DiscontinuousConvMode = DISABLE;
    hadc3.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc3.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc3.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc3.Init.NbrOfConversion = 1;
    // hadc3.Init.NbrOfDiscConversion = 1;
    hadc3.Init.DMAContinuousRequests = DISABLE;
    hadc3.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    if (HAL_ADC_Init(&hadc3) != HAL_OK) {
        ADC_Error_Handler();
    }
    /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
     */
    sConfig.Channel = ADC_CHANNEL_1;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_56CYCLES;
    if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK) {
        ADC_Error_Handler();
    }
    /* USER CODE BEGIN ADC3_Init 2 */

    /* USER CODE END ADC3_Init 2 */
}

/*
 * Fixed HAL_ADC_Start version. The one in stm32f4xx_hal_adc_ex.c prevents using
 * ADC3 when ADC1+ADC2 are set in multimode.
 */
HAL_StatusTypeDef CUSTOM_HAL_ADC_Start(ADC_HandleTypeDef *hadc) {
    __IO uint32_t counter = 0U;
    ADC_Common_TypeDef *tmpADC_Common;
    UNUSED(tmpADC_Common);

    /* Check the parameters */
    assert_param(IS_FUNCTIONAL_STATE(hadc->Init.ContinuousConvMode));
    assert_param(IS_ADC_EXT_TRIG_EDGE(hadc->Init.ExternalTrigConvEdge));

    /* Process locked */
    __HAL_LOCK(hadc);

    /* Enable the ADC peripheral */
    /* Check if ADC peripheral is disabled in order to enable it and wait during
    Tstab time the ADC's stabilization */
    if ((hadc->Instance->CR2 & ADC_CR2_ADON) != ADC_CR2_ADON) {
        /* Enable the Peripheral */
        __HAL_ADC_ENABLE(hadc);

        /* Delay for ADC stabilization time */
        /* Compute number of CPU cycles to wait for */
        counter = (ADC_STAB_DELAY_US * (SystemCoreClock / 1000000U));
        while (counter != 0U) {
            counter--;
        }
    }

    /* Start conversion if ADC is effectively enabled */
    if (HAL_IS_BIT_SET(hadc->Instance->CR2, ADC_CR2_ADON)) {
        /* Set ADC state                                                          */
        /* - Clear state bitfield related to regular group conversion results     */
        /* - Set state bitfield related to regular group operation                */
        ADC_STATE_CLR_SET(hadc->State, HAL_ADC_STATE_READY | HAL_ADC_STATE_REG_EOC | HAL_ADC_STATE_REG_OVR, HAL_ADC_STATE_REG_BUSY);

        /* If conversions on group regular are also triggering group injected,    */
        /* update ADC state.                                                      */
        if (READ_BIT(hadc->Instance->CR1, ADC_CR1_JAUTO) != RESET) {
            ADC_STATE_CLR_SET(hadc->State, HAL_ADC_STATE_INJ_EOC, HAL_ADC_STATE_INJ_BUSY);
        }

        /* State machine update: Check if an injected conversion is ongoing */
        if (HAL_IS_BIT_SET(hadc->State, HAL_ADC_STATE_INJ_BUSY)) {
            /* Reset ADC error code fields related to conversions on group regular */
            CLEAR_BIT(hadc->ErrorCode, (HAL_ADC_ERROR_OVR | HAL_ADC_ERROR_DMA));
        } else {
            /* Reset ADC all error code fields */
            ADC_CLEAR_ERRORCODE(hadc);
        }

        /* Process unlocked */
        /* Unlock before starting ADC conversions: in case of potential           */
        /* interruption, to let the process to ADC IRQ Handler.                   */
        __HAL_UNLOCK(hadc);

        /* Pointer to the common control register to which is belonging hadc    */
        /* (Depending on STM32F4 product, there may be up to 3 ADCs and 1 common */
        /* control register)                                                    */
        tmpADC_Common = ADC_COMMON_REGISTER(hadc);

        /* Clear regular group conversion flag and overrun flag */
        /* (To ensure of no unknown state from potential previous ADC operations) */
        __HAL_ADC_CLEAR_FLAG(hadc, ADC_FLAG_EOC | ADC_FLAG_OVR);

        /* Check if Multimode enabled */
        /*  if(HAL_IS_BIT_CLR(tmpADC_Common->CCR, ADC_CCR_MULTI))
             {
 #if defined(ADC2) && defined(ADC3)
                   if((hadc->Instance == ADC1) || ((hadc->Instance == ADC2) && ((ADC->CCR & ADC_CCR_MULTI_Msk) < ADC_CCR_MULTI_0)) \
                                       || ((hadc->Instance == ADC3) && ((ADC->CCR & ADC_CCR_MULTI_Msk) < ADC_CCR_MULTI_4)))
                 {
 #endif *//* ADC2 || ADC3 */
        /* if no external trigger present enable software conversion of regular channels */
        if ((hadc->Instance->CR2 & ADC_CR2_EXTEN) == RESET) {
            /* Enable the selected ADC software conversion for regular group */
            hadc->Instance->CR2 |= (uint32_t)ADC_CR2_SWSTART;
        }
        //#if defined(ADC2) && defined(ADC3)
        //          }
        //#endif /* ADC2 || ADC3 */
        /*     }
            else
          {*/
        /* if instance of handle correspond to ADC1 and  no external trigger present enable software conversion of regular channels */
        /*       if((hadc->Instance == ADC1) && ((hadc->Instance->CR2 & ADC_CR2_EXTEN) == RESET))
               {*/
        /* Enable the selected ADC software conversion for regular group */
        /*        hadc->Instance->CR2 |= (uint32_t)ADC_CR2_SWSTART;
             }
           }*/
    }

    /* Return function status */
    return HAL_OK;
}

int GetADCValue(ADC_HandleTypeDef *hadc, uint32_t Channel, int count) {

    // If we are reinitializing the same ADC between single shot and DMA mode we need to initialize for single shot here
    // TODO: Check if it's already in single shot mode to avoid reinitializing it in the same mode

    if (hadc == &hadc1) {
        MX_ADC1_Init();
    } else if (hadc == &hadc2) {
        MX_ADC2_Init();
    }

    int val = 0, v = 0;
    HAL_StatusTypeDef err = HAL_OK;
    ADC_ChannelConfTypeDef sConfig;
    sConfig.Channel = Channel;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_84CYCLES;

    if (HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK) {
        ADC_Error_Handler();
    }

    int cnt = 0;
    for (int i = 0; i < count + 1 && err == HAL_OK; i++) {

        if (hadc == &hadc3) {
            CUSTOM_HAL_ADC_Start(hadc);
        } else {
            HAL_ADC_Start(hadc);
        }

        err = HAL_ADC_PollForConversion(hadc, 1);

        if (err == HAL_OK && i > 0) { // trash the first reading as it's proved to be unreliable
            v = HAL_ADC_GetValue(hadc);
            val += v;
            cnt++;
        }

        // If we are changing between DMA and direct conversion mode we should stop the ADC here. It seems that, otherwise, the DMA mode
        // won't fire interrupts (don't know why)
        // HAL_ADC_Stop(hadc);
    }
    HAL_ADC_Stop(hadc);
    return cnt ? val / cnt : -1;
}

void ADC_DMA_Start(ADC_HandleTypeDef *hadc) {

    // LOG("ADC_DMA_START");
    if (!adc_dma_started) {

        Config_ADC_DMA();

#if (DUAL_ADC_CONV)
        uint16_t capture_length = DSP_BLOCK * 2;

        // Start dual simultaneous conversions in ADCs 1&2
        HAL_ADC_Start(&hadc2);
        HAL_ADCEx_MultiModeStart_DMA(hadc, (uint32_t *)adc_buff, capture_length);

        HAL_TIM_Base_Start_IT(&htim2); // Start ACD DMA timer

#else
        uint16_t capture_length = DSP_BLOCK * 2 * 2;
        // Start single ADC sequenced conversion
        HAL_ADC_Start_DMA(hadc, (uint32_t *)&adc_buff, capture_length);
#endif
        adc_dma_started = true;
    } else {
        // LOG(": Did nothing");
    }
    // LOG("\n");
}

void ADC_DMA_Stop(ADC_HandleTypeDef *hadc) {

    // LOG("ADC_DMA_STOP");
    if (adc_dma_started) {

        HAL_TIM_Base_Stop_IT(&htim2); // Stop ACD DMA timer
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
        hadc1_mode = 0;
        hadc2_mode = 0;
    } else {
        // LOG(": Did nothing");
    }

    // LOG("\n");
}

void setup_adcs() {

    MX_ADC3_Init();

#if ENABLE_FFT
    Config_ADC_DMA();
#endif

    // MX_ADC2_Init();
}
