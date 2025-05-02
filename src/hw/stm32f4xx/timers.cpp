//
// Created by Angel Dust on 01/11/2019.
//

#include "hw/stm32.h"
#include "timers.h"
#include "config.h"
#include <sys/_stdint.h>

TIM_HandleTypeDef htim3;  // Led blink
TIM_HandleTypeDef htim2;  // ADC DMA
TIM_HandleTypeDef htim6;  // DAC DMA
TIM_HandleTypeDef htim13; // Debouncer timer
TIM_HandleTypeDef htim14; // SD Card FIFO processing task timer

#ifdef __cplusplus
extern "C" {
#endif
// Must be defined elsewhere
extern void Error_Handler(void);
#ifdef __cplusplus
}
#endif

/*
void TIM2_IRQHandler(void) {

    HAL_GPIO_TogglePin(GPIOD,GPIO_PIN_5);
    //HAL_TIM_IRQHandler(&htim2);
}
*/

/**
 * @brief TIM_Base MSP Initialization
 * This function configures the hardware resources used in this example
 * @param htim_base: TIM_Base handle pointer
 * @retval None
 */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim_base) {
    if (htim_base->Instance == TIM2) {
        /* USER CODE BEGIN TIM2_MspInit 0 */

        /* USER CODE END TIM2_MspInit 0 */
        /* Peripheral clock enable */
        __HAL_RCC_TIM2_CLK_ENABLE();
        /*  HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
          HAL_NVIC_EnableIRQ(TIM2_IRQn);*/
        /* USER CODE BEGIN TIM2_MspInit 1 */

        /* USER CODE END TIM2_MspInit 1 */
    }
    if (htim_base->Instance == TIM6) {
        /* USER CODE BEGIN TIM6_MspInit 0 */

        /* USER CODE END TIM6_MspInit 0 */
        /* Peripheral clock enable */
        __HAL_RCC_TIM6_CLK_ENABLE();
        /* USER CODE BEGIN TIM6_MspInit 1 */

        /* USER CODE END TIM6_MspInit 1 */
    }
    if (htim_base->Instance == TIM13) {
        /* USER CODE BEGIN TIM13_MspInit 0 */

        /* USER CODE END TIM13_MspInit 0 */
        /* Peripheral clock enable */
        __HAL_RCC_TIM13_CLK_ENABLE();
        /* TIM13 interrupt Init */
        HAL_NVIC_SetPriority(TIM8_UP_TIM13_IRQn, 4, 0);
        HAL_NVIC_EnableIRQ(TIM8_UP_TIM13_IRQn);
        /* USER CODE BEGIN TIM13_MspInit 1 */

        /* USER CODE END TIM13_MspInit 1 */
    } else if (htim_base->Instance == TIM14) {
        /* USER CODE BEGIN TIM14_MspInit 0 */

        /* USER CODE END TIM14_MspInit 0 */
        /* Peripheral clock enable */
        __HAL_RCC_TIM14_CLK_ENABLE();
        /* TIM14 interrupt Init */
        HAL_NVIC_SetPriority(TIM8_TRG_COM_TIM14_IRQn, 3, 0);
        HAL_NVIC_EnableIRQ(TIM8_TRG_COM_TIM14_IRQn);
        /* USER CODE BEGIN TIM14_MspInit 1 */

        /* USER CODE END TIM14_MspInit 1 */
    } else if (htim_base->Instance == TIM3) {
        /* USER CODE BEGIN TIM3_MspInit 0 */

        /* USER CODE END TIM3_MspInit 0 */
        /* Peripheral clock enable */
        __HAL_RCC_TIM3_CLK_ENABLE();
        /* TIM3 interrupt Init */
        HAL_NVIC_SetPriority(TIM3_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(TIM3_IRQn);
        /* USER CODE BEGIN TIM3_MspInit 1 */

        /* USER CODE END TIM3_MspInit 1 */
    }
}

/**
 * @brief TIM_OC MSP Initialization
 * This function configures the hardware resources used in this example
 * @param htim_oc: TIM_OC handle pointer
 * @retval None
 */
void HAL_TIM_OC_MspInit(TIM_HandleTypeDef *htim_oc) {
}

/**
 * @brief TIM_Base MSP De-Initialization
 * This function freeze the hardware resources used in this example
 * @param htim_base: TIM_Base handle pointer
 * @retval None
 */
void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *htim_base) {
    if (htim_base->Instance == TIM2) {
        /* USER CODE BEGIN TIM2_MspDeInit 0 */

        /* USER CODE END TIM2_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_TIM2_CLK_DISABLE();
        /* USER CODE BEGIN TIM2_MspDeInit 1 */

        /* USER CODE END TIM2_MspDeInit 1 */
    } else if (htim_base->Instance == TIM6) {
        /* USER CODE BEGIN TIM6_MspDeInit 0 */

        /* USER CODE END TIM6_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_TIM6_CLK_DISABLE();
        /* USER CODE BEGIN TIM6_MspDeInit 1 */

        /* USER CODE END TIM6_MspDeInit 1 */
    } else if (htim_base->Instance == TIM13) {
        /* USER CODE BEGIN TIM13_MspDeInit 0 */

        /* USER CODE END TIM13_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_TIM13_CLK_DISABLE();
        HAL_NVIC_DisableIRQ(TIM8_UP_TIM13_IRQn);
        /* USER CODE BEGIN TIM13_MspDeInit 1 */

        /* USER CODE END TIM13_MspDeInit 1 */
    } else if (htim_base->Instance == TIM14) {
        /* USER CODE BEGIN TIM14_MspDeInit 0 */

        /* USER CODE END TIM14_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_TIM14_CLK_DISABLE();

        /* TIM14 interrupt DeInit */
        HAL_NVIC_DisableIRQ(TIM8_TRG_COM_TIM14_IRQn);
        /* USER CODE BEGIN TIM14_MspDeInit 1 */

        /* USER CODE END TIM14_MspDeInit 1 */
    } else if (htim_base->Instance == TIM3) {
        /* USER CODE BEGIN TIM3_MspDeInit 0 */

        /* USER CODE END TIM3_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_TIM3_CLK_DISABLE();

        /* TIM14 interrupt DeInit */
        HAL_NVIC_DisableIRQ(TIM3_IRQn);
        /* USER CODE BEGIN TIM3_MspDeInit 1 */

        /* USER CODE END TIM3_MspDeInit 1 */
    }
}

/**
 * @brief TIM_OC MSP De-Initialization
 * This function freeze the hardware resources used in this example
 * @param htim_oc: TIM_OC handle pointer
 * @retval None
 */
void HAL_TIM_OC_MspDeInit(TIM_HandleTypeDef *htim_oc) {
}

/* Timers associated pins initialization */
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim) {
}

/**
 * @brief TIM3 Initialization Function
 * @param None
 * @retval None
 */
void MX_TIM3_Init(void) {

    /* USER CODE BEGIN TIM3_Init 0 */

    /* USER CODE END TIM3_Init 0 */

    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    /* USER CODE BEGIN TIM3_Init 1 */

    /* USER CODE END TIM3_Init 1 */
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = CPU_TIMER_PRESCALER_MS_HZ;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 1000;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.RepetitionCounter = 0;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim3) != HAL_OK) {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK) {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN TIM3_Init 2 */

    /* USER CODE END TIM3_Init 2 */
}

/**
 * @brief TIM2 Initialization Function
 * @param None
 * @retval None
 */
void MX_TIM2_Init(void) {

    /* USER CODE BEGIN TIM2_Init 0 */

    /* USER CODE END TIM2_Init 0 */

    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    /* USER CODE BEGIN TIM2_Init 1 */

    /* USER CODE END TIM2_Init 1 */
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 180 - 1;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 10000;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK) {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_TIM_OC_Init(&htim2) != HAL_OK) {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }

    /* USER CODE BEGIN TIM2_Init 2 */

    /* USER CODE END TIM2_Init 2 */
}
/**
 * @brief TIM4 Initialization Function
 * @param None
 * @retval None
 *//*

void MX_TIM4_Init(void) {

    */
/* USER CODE BEGIN TIM4_Init 0 */ /*


     */
/* USER CODE END TIM4_Init 0 */   /*
  
  
       TIM_ClockConfigTypeDef sClockSourceConfig = {0};
       TIM_MasterConfigTypeDef sMasterConfig = {0};
       TIM_OC_InitTypeDef sConfigOC = {0};
  
       */
/* USER CODE BEGIN TIM4_Init 1 */ /*


     */
/* USER CODE END TIM4_Init 1 */   /*
  
       htim4.Instance = TIM4;
       htim4.Init.Prescaler = 3;
       htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
       htim4.Init.Period = 1000;
       htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
       htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
       if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
       {
           Error_Handler();
       }
       sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
       if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
       {
           Error_Handler();
       }
       if (HAL_TIM_OC_Init(&htim4) != HAL_OK)
       {
           Error_Handler();
       }
       sMasterConfig.MasterOutputTrigger = TIM_TRGO_OC1REF;
       sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
       if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
       {
           Error_Handler();
       }
       sConfigOC.OCMode = TIM_OCMODE_TIMING;
       sConfigOC.Pulse = 0;
       sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
       sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
       if (HAL_TIM_OC_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
       {
           Error_Handler();
       }
       */
/* USER CODE BEGIN TIM4_Init 2 */ /*


     */
/* USER CODE END TIM4_Init 2 */   /*
  
   }
   */

void MX_TIM6_Init(void) {

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
    if (HAL_TIM_Base_Init(&htim6) != HAL_OK) {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN TIM6_Init 2 */

    /* USER CODE END TIM6_Init 2 */
}

/**
 * @brief TIM14 Initialization Function
 * @param None
 * @retval None
 */
void MX_TIM14_Init(void) {

    /* USER CODE BEGIN TIM14_Init 0 */

    /* USER CODE END TIM14_Init 0 */

    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    /* USER CODE BEGIN TIM14_Init 1 */

    /* USER CODE END TIM14_Init 1 */
    htim14.Instance = TIM14;
    htim14.Init.Prescaler = CPU_TIMER_PRESCALER_MS_HZ;
    htim14.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim14.Init.Period = 1000;
    htim14.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim14.Init.RepetitionCounter = 0;
    htim14.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim14) != HAL_OK) {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim14, &sClockSourceConfig) != HAL_OK) {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim14, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN TIM14_Init 2 */

    /* USER CODE END TIM14_Init 2 */
}

void MX_TIM13_Init(void) {

    /* USER CODE BEGIN TIM13_Init 0 */

    /* USER CODE END TIM13_Init 0 */

    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    /* USER CODE BEGIN TIM13_Init 1 */

    /* USER CODE END TIM13_Init 1 */
    htim13.Instance = TIM13;
    htim13.Init.Prescaler = CPU_TIMER_PRESCALER_MS_HZ;
    htim13.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim13.Init.Period = 1000;
    htim13.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim13.Init.RepetitionCounter = 0;
    htim13.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim13) != HAL_OK) {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim13, &sClockSourceConfig) != HAL_OK) {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim13, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN TIM13_Init 2 */

    /* USER CODE END TIM13_Init 2 */
}

void update_timer(TIM_TypeDef *timer, uint32_t period, uint32_t prescaler) {
    /* Set the Autoreload value */
    timer->ARR = period - 1;

    /* Set the Prescaler value */
    timer->PSC = prescaler - 1;

    // if (IS_TIM_REPETITION_COUNTER_INSTANCE(timer))
    //{
    /* Set the Repetition Counter value */
    //    timer->RCR = Structure->RepetitionCounter;
    //}

    /* Generate an update event to reload the Prescaler
     and the repetition counter(only for TIM1 and TIM8) value immediatly */
    timer->EGR = TIM_EGR_UG;
}

void set_timer_sample_rate_NOT_INTEGER_PHASE_MATCH(TIM_TypeDef *timer, uint32_t clk_freq, uint32_t hz) {

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

    // if (IS_TIM_REPETITION_COUNTER_INSTANCE(TIM4))
    //{
    /* Set the Repetition Counter value */
    //    timer->RCR = Structure->RepetitionCounter;
    //}

    /* Generate an update event to reload the Prescaler
     and the repetition counter (only for TIM1 and TIM8) value immediatly */
    timer->EGR = TIM_EGR_UG;
}

void set_timer_sample_rate(TIM_TypeDef *timer, uint32_t clk_freq, uint32_t hz) {
    uint32_t target_div = clk_freq / hz;

    uint32_t best_psc = 0;
    uint32_t best_arr = 0;
    uint32_t min_error = 0xFFFFFFFF;

    for (uint32_t psc = 0; psc <= 0xFFFF; ++psc) {
        uint32_t denom = psc + 1;

        uint32_t arr = target_div / denom;

        if (arr == 0 || arr > 0x10000) {
            continue;
        }

        uint32_t actual_freq = clk_freq / (denom * arr);
        uint32_t error = (actual_freq > hz) ? (actual_freq - hz) : (hz - actual_freq);

        if (error < min_error) {
            min_error = error;
            best_psc = psc;
            best_arr = arr - 1;
            if (error == 0) {
                break; // perfect match found
            }
        }
    }

    timer->PSC = best_psc;
    timer->ARR = best_arr;
    timer->EGR = TIM_EGR_UG;
}

void setup_timers() {

    MX_TIM3_Init();  // LED timer
    MX_TIM2_Init();  // ADC DMA timer
    MX_TIM6_Init();  // DAC DMA timer
    MX_TIM13_Init(); // Input pin debouncer timer (TODO: I think it's initialized in the constuctor of the InputPinController)
    MX_TIM14_Init(); // IO task timer

    // Freeze timers when debugging
    HAL_DBGMCU_EnableDBGStandbyMode();
    HAL_DBGMCU_EnableDBGStopMode();

    DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_TIM2_STOP | DBGMCU_APB1_FZ_DBG_TIM6_STOP | DBGMCU_APB1_FZ_DBG_TIM14_STOP | DBGMCU_APB1_FZ_DBG_TIM3_STOP;
    ;

    // Calculate the pre-scaler and period for the config sample rate
    set_timer_sample_rate(ADC_DMA_TIMER, ADC_DMA_TIMER_CLOCK_HZ, fft_params.sample_freq);
}
