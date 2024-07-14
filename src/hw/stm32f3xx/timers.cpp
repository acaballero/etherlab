//
// Created by Angel Dust on 01/11/2019.
//

#include "timers.h"
#include "config.h"

TIM_HandleTypeDef htim3; // Led blink
TIM_HandleTypeDef htim4; // ADC DMA
TIM_HandleTypeDef htim6; // DAC DMA
TIM_HandleTypeDef htim15; // SD Card FIFO processing task timer

#ifdef __cplusplus
extern "C" {
#endif
// Must be defined elsewhere
extern void Error_Handler(void);
#ifdef __cplusplus
}
#endif



/**
* @brief TIM_Base MSP Initialization
* This function configures the hardware resources used in this example
* @param htim_base: TIM_Base handle pointer
* @retval None
*/
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim_base) {
    if (htim_base->Instance == TIM4) {
        /* USER CODE BEGIN TIM4_MspInit 0 */

        /* USER CODE END TIM4_MspInit 0 */
        /* Peripheral clock enable */
        __HAL_RCC_TIM4_CLK_ENABLE();
        /* USER CODE BEGIN TIM4_MspInit 1 */

        /* USER CODE END TIM4_MspInit 1 */
    }
    if (htim_base->Instance == TIM6) {
        /* USER CODE BEGIN TIM4_MspInit 0 */

        /* USER CODE END TIM4_MspInit 0 */
        /* Peripheral clock enable */
        __HAL_RCC_TIM6_CLK_ENABLE();
        /* USER CODE BEGIN TIM4_MspInit 1 */

        /* USER CODE END TIM4_MspInit 1 */
    }
    else if(htim_base->Instance==TIM15)
    {
        /* USER CODE BEGIN TIM15_MspInit 0 */

        /* USER CODE END TIM15_MspInit 0 */
        /* Peripheral clock enable */
        __HAL_RCC_TIM15_CLK_ENABLE();
        /* TIM15 interrupt Init */
        HAL_NVIC_SetPriority(TIM1_BRK_TIM15_IRQn, 2, 0);
        HAL_NVIC_EnableIRQ(TIM1_BRK_TIM15_IRQn);
        /* USER CODE BEGIN TIM15_MspInit 1 */

        /* USER CODE END TIM15_MspInit 1 */
    }

}

/**
* @brief TIM_OC MSP Initialization
* This function configures the hardware resources used in this example
* @param htim_oc: TIM_OC handle pointer
* @retval None
*/
void HAL_TIM_OC_MspInit(TIM_HandleTypeDef *htim_oc) {

    if (htim_oc->Instance == TIM2) {
        /* USER CODE BEGIN TIM2_MspInit 0 */

        /* USER CODE END TIM2_MspInit 0 */
        /* Peripheral clock enable */
        __HAL_RCC_TIM2_CLK_ENABLE();
        /* TIM2 interrupt Init */
        HAL_NVIC_SetPriority(TIM2_IRQn, 10, 0);
        HAL_NVIC_EnableIRQ(TIM2_IRQn);
        /* USER CODE BEGIN TIM2_MspInit 1 */

        /* USER CODE END TIM2_MspInit 1 */
    }
    else if (htim_oc->Instance == TIM3) {
        /* USER CODE BEGIN TIM3_MspInit 0 */

        /* USER CODE END TIM3_MspInit 0 */
        /* Peripheral clock enable */
        __HAL_RCC_TIM3_CLK_ENABLE();
        /* USER CODE BEGIN TIM3_MspInit 1 */

        /* USER CODE END TIM3_MspInit 1 */
    }

}

/**
* @brief TIM_Base MSP De-Initialization
* This function freeze the hardware resources used in this example
* @param htim_base: TIM_Base handle pointer
* @retval None
*/
void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *htim_base) {
    if (htim_base->Instance == TIM4) {
        /* USER CODE BEGIN TIM4_MspDeInit 0 */

        /* USER CODE END TIM4_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_TIM4_CLK_DISABLE();
        /* USER CODE BEGIN TIM4_MspDeInit 1 */

        /* USER CODE END TIM4_MspDeInit 1 */
    }
    else if(htim_base->Instance==TIM6)
    {
        /* USER CODE BEGIN TIM6_MspDeInit 0 */

        /* USER CODE END TIM6_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_TIM6_CLK_DISABLE();
        /* USER CODE BEGIN TIM6_MspDeInit 1 */

        /* USER CODE END TIM6_MspDeInit 1 */
    }
    else if(htim_base->Instance==TIM15)
    {
        /* USER CODE BEGIN TIM15_MspDeInit 0 */

        /* USER CODE END TIM15_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_TIM15_CLK_DISABLE();

        /* TIM15 interrupt DeInit */
        HAL_NVIC_DisableIRQ(TIM1_BRK_TIM15_IRQn);
        /* USER CODE BEGIN TIM15_MspDeInit 1 */

        /* USER CODE END TIM15_MspDeInit 1 */
    }

}

/**
* @brief TIM_OC MSP De-Initialization
* This function freeze the hardware resources used in this example
* @param htim_oc: TIM_OC handle pointer
* @retval None
*/
void HAL_TIM_OC_MspDeInit(TIM_HandleTypeDef *htim_oc) {

    if (htim_oc->Instance == TIM3) {
        /* USER CODE BEGIN TIM3_MspDeInit 0 */

        /* USER CODE END TIM3_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_TIM3_CLK_DISABLE();
        /* USER CODE BEGIN TIM3_MspDeInit 1 */

        /* USER CODE END TIM3_MspDeInit 1 */
    }

}


/* Timers associated pins initialization */
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (htim->Instance == TIM3) {
        /* USER CODE BEGIN TIM3_MspPostInit 0 */

        /* USER CODE END TIM3_MspPostInit 0 */
        __HAL_RCC_GPIOA_CLK_ENABLE();
        /**TIM3 GPIO Configuration
        PA6     ------> TIM3_CH1
        */
        GPIO_InitStruct.Pin = GPIO_PIN_6;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* USER CODE BEGIN TIM3_MspPostInit 1 */

        /* USER CODE END TIM3_MspPostInit 1 */
    } else if (htim->Instance == TIM4) {

        /* USER CODE BEGIN TIM4_MspPostInit 0 */

        /* USER CODE END TIM4_MspPostInit 0 */

        __HAL_RCC_GPIOB_CLK_ENABLE();
        /**TIM4 GPIO Configuration
        PB9     ------> TIM4_CH4
        */
        GPIO_InitStruct.Pin = GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF2_TIM4;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* USER CODE BEGIN TIM4_MspPostInit 1 */

        /* USER CODE END TIM4_MspPostInit 1 */
    }

}


/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
void MX_TIM3_Init(void) {

    /* USER CODE BEGIN TIM3_Init 0 */

    /* USER CODE END TIM3_Init 0 */

    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};

    /* USER CODE BEGIN TIM3_Init 1 */

    /* USER CODE END TIM3_Init 1 */
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = CPU_TIMER_PRESCALER_MS_HZ;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 100;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_OC_Init(&htim3) != HAL_OK) {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }
    sConfigOC.OCMode = TIM_OCMODE_TOGGLE;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_OC_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN TIM3_Init 2 */

    /* USER CODE END TIM3_Init 2 */
    HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
void MX_TIM4_Init(void) {

    /* USER CODE BEGIN TIM4_Init 0 */

    /* USER CODE END TIM4_Init 0 */

    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};

    /* USER CODE BEGIN TIM4_Init 1 */

    /* USER CODE END TIM4_Init 1 */
    htim4.Instance = TIM4;
    htim4.Init.Prescaler = 4 - 1;
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = 1000;
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE; // Enabled: The period changes after an UEV event (timer overflow) happens. Prevents ARR shadow register to be updated with a lower value than the timer's actual value
    if (HAL_TIM_Base_Init(&htim4) != HAL_OK) {
        Error_Handler();
    }


    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_TIM_OC_Init(&htim4) != HAL_OK) {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }
    sConfigOC.OCMode = TIM_OCMODE_TIMING;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

    // Channel 4 will trigger the ADC
    if (HAL_TIM_OC_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_4) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN TIM4_Init 2 */


    /* USER CODE END TIM4_Init 2 */

    // Timers pins initialization
    //HAL_TIM_MspPostInit(&htim4);
}


void MX_TIM6_Init(void)
{

    /* USER CODE BEGIN TIM6_Init 0 */

    /* USER CODE END TIM6_Init 0 */

    TIM_MasterConfigTypeDef sMasterConfig = {0};

    /* USER CODE BEGIN TIM6_Init 1 */

    /* USER CODE END TIM6_Init 1 */
    htim6.Instance = TIM6;
    htim6.Init.Prescaler = 10;
    htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim6.Init.Period = 1000;
    htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
    {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN TIM6_Init 2 */

    /* USER CODE END TIM6_Init 2 */

}

/**
  * @brief TIM15 Initialization Function
  * @param None
  * @retval None
  */
void MX_TIM15_Init(void)
{

    /* USER CODE BEGIN TIM15_Init 0 */

    /* USER CODE END TIM15_Init 0 */

    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    /* USER CODE BEGIN TIM15_Init 1 */

    /* USER CODE END TIM15_Init 1 */
    htim15.Instance = TIM15;
    htim15.Init.Prescaler = 72000;
    htim15.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim15.Init.Period = 1000;
    htim15.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim15.Init.RepetitionCounter = 0;
    htim15.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim15) != HAL_OK)
    {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim15, &sClockSourceConfig) != HAL_OK)
    {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim15, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN TIM15_Init 2 */

    /* USER CODE END TIM15_Init 2 */

}

// timer frequency is  Core Freq Mhz/(count*prescaler) so, with a fixed prescaler of Core Freq / 10000, period_ms = (count*Core Freq / 10000*1000)/Core Freq -> count = period_ms*10
void update_timer(TIM_TypeDef *timer, uint32_t period, uint32_t prescaler) {
    /* Set the Autoreload value */
    timer->ARR = period - 1;

    /* Set the Prescaler value */
    timer->PSC = prescaler - 1;

    //if (IS_TIM_REPETITION_COUNTER_INSTANCE(timer))
    //{
    /* Set the Repetition Counter value */
    //    timer->RCR = Structure->RepetitionCounter;
    //}

    /* Generate an update event to reload the Prescaler
     and the repetition counter(only for TIM1 and TIM8) value immediatly */
    timer->EGR = TIM_EGR_UG;
}


void set_timer_sample_rate(TIM_TypeDef *timer,uint32_t clk_freq,uint32_t hz) {

    // Prescaler and period (ARR) formula
    // (PSC+1)*(ARR+1) = TIMclk/SampleFrequency

    uint32_t prescaler = 1;
    uint32_t period = clk_freq / hz;

    while (period > 0xFFFF) {
        // if we can divide exactly, do that first
        if (period % 5 == 0) {
            prescaler *= 5;
            period /= 5;
        } else if (period % 3 == 0) {
            prescaler *= 3;
            period /= 3;
        } else {
            // may not divide exactly, but loses minimal precision
            prescaler <<= 1U;
            period >>= 1U;
        }
    }

    /* Set the Autoreload value */
    timer->ARR = period - 1;

    /* Set the Prescaler value */
    timer->PSC = prescaler - 1;

    //if (IS_TIM_REPETITION_COUNTER_INSTANCE(TIM4))
    //{
    /* Set the Repetition Counter value */
    //    timer->RCR = Structure->RepetitionCounter;
    //}

    /* Generate an update event to reload the Prescaler
     and the repetition counter (only for TIM1 and TIM8) value immediatly */
    timer->EGR = TIM_EGR_UG;
}


void setup_timers() {
    MX_TIM4_Init(); // ADC DMA timer
    MX_TIM6_Init(); // DAC DMA timer
    MX_TIM15_Init(); // IO task timer

#if !ENABLE_SD_CARD
    MX_TIM3_Init(); // Timer for blinking led. If the SD card is in use, the led pin will be used as MISO
#endif

    // Freeze timers when debugging
    HAL_DBGMCU_EnableDBGStandbyMode();
    HAL_DBGMCU_EnableDBGStopMode();

    DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_TIM4_STOP | DBGMCU_APB1_FZ_DBG_TIM6_STOP;
    DBGMCU->APB1FZ |= DBGMCU_APB2_FZ_DBG_TIM15_STOP;

#if ENABLE_FFT
    set_timer_sample_rate(ADC_DMA_TIMER,ADC_DMA_TIMER_CLOCK_HZ,fft_params.sample_freq); // Calculate the prescaler and period for the config sample rate
#endif

}



