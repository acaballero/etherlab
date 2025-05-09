//
// Created by Angel Dust on 20/05/2021.
//
#include "connectivity.h"
#include "dma.h"
#include "gpio.h"
#include "handlers.h"
#include "mcp23017.h"
#include "stm32f4xx_hal_dma.h"
#include <stm32f4xx.h>

// IO Expanders
i2cbitbang i2cport01(I2CBB_MCP23017,
                     MCP23017_ADDRESS_20 << 1); // We shift the address left because this is a
                                                // 7-bit, left aligned addressed device
i2cbitbang i2cport02(I2CBB_MCP23017, MCP23017_ADDRESS_24 << 1);

// Front panel IO expander
i2cbitbang i2cport03(I2CBB_MCP23017, MCP23017_ADDRESS_22 << 1);

SPI_HandleTypeDef hspi2;
SPI_HandleTypeDef hspi4;
I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;

SD_HandleTypeDef hsd;

extern void Error_Handler();

/*!
 * \brief Initialize the SWO trace port for debug message printing
 * \param portBits Port bit mask to be configured
 * \param cpuCoreFreqHz CPU core clock frequency in Hz
 */
void SWO_Init(uint32_t portBits, uint32_t cpuCoreFreqHz) {
    uint32_t SWOSpeed = 64000;                              /* default 64k baud rate */
    uint32_t SWOPrescaler = (cpuCoreFreqHz / SWOSpeed) - 1; /* SWOSpeed in Hz, note that cpuCoreFreqHz is
                                                               expected to be match the CPU core clock */

    CoreDebug->DEMCR = CoreDebug_DEMCR_TRCENA_Msk;               /* enable trace in core debug */
    *((volatile unsigned *)(ITM_BASE + 0x400F0)) = 0x00000002;   /* "Selected PIN Protocol Register": Select which protocol to
                                                                    use for trace output (2: SWO NRZ, 1: SWO Manchester
                                                                    encoding) */
    *((volatile unsigned *)(ITM_BASE + 0x40010)) = SWOPrescaler; /* "Async Clock Prescaler Register". Scale the baud rate of
                                                                    the asynchronous output */
    *((volatile unsigned *)(ITM_BASE + 0x00FB0)) = 0xC5ACCE55;   /* ITM Lock Access Register, C5ACCE55 enables more write
                                                                    access to Control Register 0xE00 :: 0xFFC */
    ITM->TCR = ITM_TCR_TraceBusID_Msk | ITM_TCR_SWOENA_Msk | ITM_TCR_SYNCENA_Msk | ITM_TCR_ITMENA_Msk; /* ITM Trace Control Register */
    ITM->TPR = ITM_TPR_PRIVMASK_Msk;                                                                   /* ITM Trace Privilege Register */
    ITM->TER = portBits;                                                                               /* ITM Trace Enable Register. Enabled tracing on
                                                                                                          stimulus ports. One bit per stimulus port. */
    *((volatile unsigned *)(ITM_BASE + 0x01000)) = 0x400003FE;                                         /* DWT_CTRL */
    *((volatile unsigned *)(ITM_BASE + 0x40304)) = 0x00000100;                                         /* Formatter and Flush Control Register */
}

/**
 * @brief I2C MSP Initialization
 * This function configures the hardware resources used in this example
 * @param hi2c: I2C handle pointer
 * @retval None
 */
void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (hi2c->Instance == I2C1) {
        /* USER CODE BEGIN I2C1_MspInit 0 */

        /* USER CODE END I2C1_MspInit 0 */

        __HAL_RCC_GPIOB_CLK_ENABLE();
        /**I2C1 GPIO Configuration
        PB6     ------> I2C1_SCL
        PB7     ------> I2C1_SDA
        */
        GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* Peripheral clock enable */
        __HAL_RCC_I2C1_CLK_ENABLE();
        /* USER CODE BEGIN I2C1_MspInit 1 */

        /* USER CODE END I2C1_MspInit 1 */
    } else if (hi2c->Instance == I2C2) {
        /* USER CODE BEGIN I2C2_MspInit 0 */

        /* USER CODE END I2C2_MspInit 0 */

        __HAL_RCC_GPIOB_CLK_ENABLE();
        /**I2C2 GPIO Configuration
        PB10     ------> I2C2_SCL
        PB11     ------> I2C2_SDA
        */
        GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C2;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* Peripheral clock enable */
        __HAL_RCC_I2C2_CLK_ENABLE();
        /* USER CODE BEGIN I2C2_MspInit 1 */

        /* USER CODE END I2C2_MspInit 1 */
    }
}

/**
 * @brief I2C MSP De-Initialization
 * This function freeze the hardware resources used in this example
 * @param hi2c: I2C handle pointer
 * @retval None
 */
void HAL_I2C_MspDeInit(I2C_HandleTypeDef *hi2c) {
    if (hi2c->Instance == I2C1) {
        /* USER CODE BEGIN I2C1_MspDeInit 0 */

        /* USER CODE END I2C1_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_I2C1_CLK_DISABLE();

        /**I2C1 GPIO Configuration
        PB6     ------> I2C1_SCL
        PB7     ------> I2C1_SDA
        */
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6);

        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_7);

        /* USER CODE BEGIN I2C1_MspDeInit 1 */

        /* USER CODE END I2C1_MspDeInit 1 */
    } else if (hi2c->Instance == I2C2) {
        /* USER CODE BEGIN I2C2_MspDeInit 0 */

        /* USER CODE END I2C2_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_I2C2_CLK_DISABLE();

        /**I2C2 GPIO Configuration
        PB10     ------> I2C2_SCL
        PB11     ------> I2C2_SDA
        */
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10);

        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_11);

        /* USER CODE BEGIN I2C2_MspDeInit 1 */

        /* USER CODE END I2C2_MspDeInit 1 */
    }
}

/**
 * @brief SPI MSP Initialization
 * This function configures the hardware resources used in this example
 * @param hspi: SPI handle pointer
 * @retval None
 */

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi) {

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (hspi->Instance == SPI2) {
        /* USER CODE BEGIN SPI2_MspInit 0 */

        /* USER CODE END SPI2_MspInit 0 */
        /* Peripheral clock enable */
        __HAL_RCC_SPI2_CLK_ENABLE();

        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_GPIOD_CLK_ENABLE();
        /**SPI2 GPIO Configuration
        PC3     ------> SPI2_MOSI
        PD3     ------> SPI2_SCK
        */
        GPIO_InitStruct.Pin = GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; // GPIO_SPEED_FREQ_VERY_HIGH
                                                     //  Low speed reduces EMI
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM; // GPIO_SPEED_FREQ_VERY_HIGH
                                                        //  Low speed reduces EMI
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
        HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

        /* SPI2 DMA Init */
        /* SPI2_TX Init */
        hdma_spi2_tx.Instance = DMA1_Stream4;
        hdma_spi2_tx.Init.Channel = DMA_CHANNEL_0;
        hdma_spi2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_spi2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_spi2_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_spi2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_spi2_tx.Init.MemDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_spi2_tx.Init.Mode = DMA_NORMAL;
        hdma_spi2_tx.Init.Priority = DMA_PRIORITY_LOW;

        /* FIFO settings */
        // hdma_spi2_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

        hdma_spi2_tx.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
        hdma_spi2_tx.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;

        hdma_spi2_tx.Init.MemBurst = DMA_MBURST_INC16;
        hdma_spi2_tx.Init.PeriphBurst = DMA_PBURST_SINGLE;

        if (HAL_DMA_Init(&hdma_spi2_tx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(hspi, hdmatx, hdma_spi2_tx);

        /* SPI2 interrupt Init */
        // HAL_NVIC_SetPriority(SPI2_IRQn, 0, 0);
        // HAL_NVIC_EnableIRQ(SPI2_IRQn);

        /* USER CODE BEGIN SPI2_MspInit 1 */

        /* USER CODE END SPI2_MspInit 1 */
    } else if (hspi->Instance == SPI4) {
        /* USER CODE BEGIN SPI4_MspInit 0 */

        /* USER CODE END SPI4_MspInit 0 */
        /* Peripheral clock enable */
        __HAL_RCC_SPI4_CLK_ENABLE();

        __HAL_RCC_GPIOE_CLK_ENABLE();
        /**SPI4 GPIO Configuration
        PE2     ------> SPI4_SCK
        PE5     ------> SPI4_MISO
        PE6     ------> SPI4_MOSI
        */
        GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_5 | GPIO_PIN_6;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; // GPIO_SPEED_FREQ_VERY_HIGH;
                                                      // // Low speed reduces EMI
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI4;
        HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

        /* USER CODE BEGIN SPI4_MspInit 1 */

        /* USER CODE END SPI4_MspInit 1 */
    }
}

/**
 * @brief SPI MSP De-Initialization
 * This function freeze the hardware resources used in this example
 * @param hspi: SPI handle pointer
 * @retval None
 */
void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI2) {
        /* USER CODE BEGIN SPI2_MspDeInit 0 */

        /* USER CODE END SPI2_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_SPI2_CLK_DISABLE();

        /**SPI2 GPIO Configuration
        PC3     ------> SPI2_MOSI
        PD3     ------> SPI2_SCK
        */
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_3);

        HAL_GPIO_DeInit(GPIOD, GPIO_PIN_3);

        /* SPI2 DMA DeInit */
        HAL_DMA_DeInit(hspi->hdmatx);
        /* USER CODE BEGIN SPI2_MspDeInit 1 */

        /* USER CODE END SPI2_MspDeInit 1 */
    } else if (hspi->Instance == SPI4) {
        /* USER CODE BEGIN SPI4_MspDeInit 0 */

        /* USER CODE END SPI4_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_SPI4_CLK_DISABLE();

        /**SPI4 GPIO Configuration
        PE2     ------> SPI4_SCK
        PE5     ------> SPI4_MISO
        PE6     ------> SPI4_MOSI
        */
        HAL_GPIO_DeInit(GPIOE, GPIO_PIN_2 | GPIO_PIN_5 | GPIO_PIN_6);

        /* USER CODE BEGIN SPI4_MspDeInit 1 */

        /* USER CODE END SPI4_MspDeInit 1 */
    }
}

void MX_SPI2_Init(void) {

    /* USER CODE BEGIN SPI2_Init 0 */

    /* USER CODE END SPI2_Init 0 */

    /* USER CODE BEGIN SPI2_Init 1 */

    /* USER CODE END SPI2_Init 1 */
    /* SPI2 parameter configuration*/
    hspi2.Instance = SPI2;
    hspi2.Init.Mode = SPI_MODE_MASTER;
    hspi2.Init.Direction = SPI_DIRECTION_2LINES;
    hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi2.Init.NSS = SPI_NSS_SOFT;
    hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi2.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi2) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN SPI2_Init 2 */

    /* USER CODE END SPI2_Init 2 */
}

/**
 * @brief SPI4 Initialization Function
 * @param None
 * @retval None
 */
void MX_SPI4_Init(void) {

    /* USER CODE BEGIN SPI4_Init 0 */

    /* USER CODE END SPI4_Init 0 */

    /* USER CODE BEGIN SPI4_Init 1 */

    /* USER CODE END SPI4_Init 1 */
    /* SPI4 parameter configuration*/
    hspi4.Instance = SPI4;
    hspi4.Init.Mode = SPI_MODE_MASTER;
    hspi4.Init.Direction = SPI_DIRECTION_2LINES;
    hspi4.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi4.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi4.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi4.Init.NSS = SPI_NSS_SOFT;
    hspi4.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
    hspi4.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi4.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi4.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi4.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi4) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN SPI4_Init 2 */

    /* USER CODE END SPI4_Init 2 */
}

/**
 * @brief I2C1 Initialization Function
 * @param None
 * @retval None
 */
void MX_I2C1_Init(void) {

    /* USER CODE BEGIN I2C1_Init 0 */

    /* USER CODE END I2C1_Init 0 */

    /* USER CODE BEGIN I2C1_Init 1 */

    /* USER CODE END I2C1_Init 1 */
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        Error_Handler();
    }
    /** Configure Analogue filter
     */
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
        Error_Handler();
    }
    /** Configure Digital filter
     */
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN I2C1_Init 2 */

    /* USER CODE END I2C1_Init 2 */
}

/**
 * @brief I2C2 Initialization Function
 * @param None
 * @retval None
 */
void MX_I2C2_Init(void) {

    /* USER CODE BEGIN I2C2_Init 0 */

    /* USER CODE END I2C2_Init 0 */

    /* USER CODE BEGIN I2C2_Init 1 */

    /* USER CODE END I2C2_Init 1 */
    hi2c2.Instance = I2C2;
    hi2c2.Init.ClockSpeed = 400000;
    hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c2.Init.OwnAddress1 = 0;
    hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c2.Init.OwnAddress2 = 0;
    hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c2) != HAL_OK) {
        Error_Handler();
    }
    /** Configure Analogue filter
     */
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
        Error_Handler();
    }
    //    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_DISABLE) !=
    //    HAL_OK) {
    //        Error_Handler();
    //    }
    /** Configure Digital filter
     */
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN I2C2_Init 2 */

    /* USER CODE END I2C2_Init 2 */
}

/**
 * @brief SDIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_SDIO_SD_Init(void) {
    /* USER CODE BEGIN SDIO_Init 0 */

    /* USER CODE END SDIO_Init 0 */

    /* USER CODE BEGIN SDIO_Init 1 */

    /* USER CODE END SDIO_Init 1 */
    hsd.Instance = SDIO;
    hsd.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
    hsd.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
    hsd.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_ENABLE;
    hsd.Init.BusWide = SDIO_BUS_WIDE_1B;
    hsd.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
    hsd.Init.ClockDiv = 1;
    if (HAL_SD_Init(&hsd) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_SD_ConfigWideBusOperation(&hsd, SDIO_BUS_WIDE_4B) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN SDIO_Init 2 */

    /* USER CODE END SDIO_Init 2 */
}

/**
 * @brief SD MSP Initialization
 * This function configures the hardware resources used in this example
 * @param hsd: SD handle pointer
 * @retval Non   e
 */
void HAL_SD_MspInit(SD_HandleTypeDef *hsd) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (hsd->Instance == SDIO) {
        /* USER CODE BEGIN SDIO_MspInit 0 */

        /* USER CODE END SDIO_MspInit 0 */
        /* Peripheral clock enable */
        __HAL_RCC_SDIO_CLK_ENABLE();

        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_GPIOD_CLK_ENABLE();
        /**SDIO GPIO Configuration
        PC8     ------> SDIO_D0
        PC9     ------> SDIO_D1
        PC10     ------> SDIO_D2
        PC11     ------> SDIO_D3
        PC12     ------> SDIO_CK
        PD2     ------> SDIO_CMD
        */
        GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF12_SDIO;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = GPIO_PIN_2;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; // LOW for reduced EMI
        GPIO_InitStruct.Alternate = GPIO_AF12_SDIO;
        HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

        /* SDIO DMA Init */
        /* SDIO_RX Init */
        hdma_sdio_rx.Instance = DMA2_Stream3;
        hdma_sdio_rx.Init.Channel = DMA_CHANNEL_4;
        hdma_sdio_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_sdio_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_sdio_rx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_sdio_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
        hdma_sdio_rx.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
        hdma_sdio_rx.Init.Mode = DMA_PFCTRL;
        hdma_sdio_rx.Init.Priority = DMA_PRIORITY_MEDIUM;
        hdma_sdio_rx.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
        hdma_sdio_rx.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
        hdma_sdio_rx.Init.MemBurst = DMA_MBURST_INC4;
        hdma_sdio_rx.Init.PeriphBurst = DMA_PBURST_INC4;

        if (HAL_DMA_Init(&hdma_sdio_rx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(hsd, hdmarx, hdma_sdio_rx);

        /* SDIO_TX Init */
        hdma_sdio_tx.Instance = DMA2_Stream6;
        hdma_sdio_tx.Init.Channel = DMA_CHANNEL_4;
        hdma_sdio_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_sdio_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_sdio_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_sdio_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
        hdma_sdio_tx.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
        hdma_sdio_tx.Init.Mode = DMA_PFCTRL;
        hdma_sdio_tx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
        hdma_sdio_tx.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
        hdma_sdio_tx.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
        hdma_sdio_tx.Init.MemBurst = DMA_MBURST_INC4;
        hdma_sdio_tx.Init.PeriphBurst = DMA_PBURST_INC4;
        if (HAL_DMA_Init(&hdma_sdio_tx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(hsd, hdmatx, hdma_sdio_tx);

        HAL_NVIC_SetPriority(SDIO_IRQn, 2, 0);
        HAL_NVIC_EnableIRQ(SDIO_IRQn);

        /* USER CODE BEGIN SDIO_MspInit 1 */

        /* USER CODE END SDIO_MspInit 1 */
    }
}

/**
 * @brief SD MSP De-Initialization
 * This function freeze the hardware resources used in this example
 * @param hsd: SD handle pointer
 * @retval None
 */
void HAL_SD_MspDeInit(SD_HandleTypeDef *hsd) {
    if (hsd->Instance == SDIO) {
        /* USER CODE BEGIN SDIO_MspDeInit 0 */

        /* USER CODE END SDIO_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_SDIO_CLK_DISABLE();

        /**SDIO GPIO Configuration
        PC8     ------> SDIO_D0
        PC9     ------> SDIO_D1
        PC10     ------> SDIO_D2
        PC11     ------> SDIO_D3
        PC12     ------> SDIO_CK
        PD2     ------> SDIO_CMD
        */
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12);

        HAL_GPIO_DeInit(GPIOD, GPIO_PIN_2);

        /* USER CODE BEGIN SDIO_MspDeInit 1 */

        /* USER CODE END SDIO_MspDeInit 1 */
    }
}

void BitBangI2C_setup() {

    // Make sure the MCP23017 is powered
    // config.power_ctrl |= POWCRL_P5;
    // setPowerCtrl();

    i2cport01.setSpeed(SPEED_10k);
    i2cport02.setSpeed(SPEED_10k);
    i2cport03.setSpeed(SPEED_10k);

    hmcp01.i2cbb = &i2cport01;

    mcp23017_iodir(&hmcp01, MCP23017_PORTA, MCP23017_IODIR_ALL_OUTPUT);
    mcp23017_iodir(&hmcp01, MCP23017_PORTB, MCP23017_IODIR_ALL_OUTPUT);

    hmcp02.i2cbb = &i2cport02;

    mcp23017_iodir(&hmcp02, MCP23017_PORTA, MCP23017_IODIR_ALL_OUTPUT);
    mcp23017_iodir(&hmcp02, MCP23017_PORTB, MCP23017_IODIR_ALL_OUTPUT);

    hmcp03.i2cbb = &i2cport03;

    mcp23017_iodir(&hmcp03, MCP23017_PORTA, 0b00111111);
    mcp23017_iodir(&hmcp03, MCP23017_PORTB, 0b00111111);

    // Activate internal pull-up
    mcp23017_writereg(&hmcp03, REGISTER_GPPUA, 0b00111111);
    mcp23017_writereg(&hmcp03, REGISTER_GPPUB, 0b00111111);

    // Invert polarity
    mcp23017_writereg(&hmcp03, REGISTER_IPOLA, 0b00111111);
    mcp23017_writereg(&hmcp03, REGISTER_IPOLB, 0b00111111);

    // Configure interrupts
    mcp23017_writereg(&hmcp03, REGISTER_IOCONA,
                      0b01000000); // MIRROR + open drain + polarity
    mcp23017_writereg(&hmcp03, REGISTER_IOCONB, 0b01000000);

    // Interrupt enable
    mcp23017_writereg(&hmcp03, REGISTER_GPINTENA, 0b00111111);
    mcp23017_writereg(&hmcp03, REGISTER_GPINTENB, 0b00111111);

    // Read interrupt capture ports to clear them
    uint8_t d;
    mcp23017_read(&hmcp03, REGISTER_INTCAPA, &d);
    mcp23017_read(&hmcp03, REGISTER_INTCAPB, &d);

    // TEST
    // hmcp03.gpio[MCP23017_PORTA] = 0x0F;
    // hmcp03.gpio[MCP23017_PORTB] = 0xF0;
    // mcp23017_write_gpio(&hmcp03, MCP23017_PORTA);
    // mcp23017_write_gpio(&hmcp03, MCP23017_PORTB);
    //    hmcp.gpio[MCP23017_PORTA]=0xF0;
    //    hmcp.gpio[MCP23017_PORTB]=0x0F;
    //    mcp23017_write_gpio(&hmcp,MCP23017_PORTA);
    //    mcp23017_write_gpio(&hmcp,MCP23017_PORTB);
}

void setup_connectivity() {

    /* SPI */
    MX_SPI2_Init();
    MX_SPI4_Init();

    /* I2C */
    // MX_I2C1_Init(); // This is the available SPI at the pin header. We don't
    // use it unless needed
    MX_I2C2_Init();

#if ENABLE_SD_CARD
    /* SDIO (SD CARD) */
    MX_SDIO_SD_Init();
#endif
}

void SDIO_IRQHandler(void) {
    HAL_SD_IRQHandler(&hsd);
}

/*
 * SPI2 general interrupt handler
 * This is required if the DMA FIFO is enabled and we are transferring to the
 * peripheral. Otherwise, the transfer complete callback handler won't be fired,
 * since it is fired from HAL_SPI_IRQHandler only when the FIFO is empty
 */
void SPI2_IRQHandler(void) {
    HAL_SPI_IRQHandler(&hspi2);
}
