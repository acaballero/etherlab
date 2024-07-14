//
// Created by Angel Dust on 20/05/2021.
//

#include <stm32f4xx.h>
#include "gpio.h"
#include "hw/hw_config.h"
#include "../lib/MCP23017/mcp23017.h"

// MCP23017 GPIO expansion port descriptors
MCP23017_HandleTypeDef hmcp01;
MCP23017_HandleTypeDef hmcp02;

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();


    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2, GPIO_PIN_RESET);

    /*Configure GPIO pins : PB0 PB1 PB2  */

    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2;

    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // PB10 & PB11 will be used (and initialized) for bitbanged I2C in its class

#if USB_ENABLED
    /*Configure GPIO pins :  PA10: USB Enabled */
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#else
    /* Make PA10-PA12 outputs for debugging */
    GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
#endif



    /*Configure GPIO Power control pins (shift register) */
    GPIO_InitStruct.Pin = POW_CTRL_SET_PIN | POW_CTRL_CLK_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(POW_CTRL_CLK_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = POW_CTRL_DATA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(POW_CTRL_DATA_PORT, &GPIO_InitStruct);

#if ENABLE_SD_CARD
    /* SD CARD CS PIN */
    GPIO_InitStruct.Pin = GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
#endif

    /* EXTI interrupt init*/
    HAL_NVIC_SetPriority(EXTI1_IRQn, 14, 0);
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);

    /* EXTI interrupt init*/
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

    /* Enable interrupts for 5:9 handlers */
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

    // SPI pins pins are enabled in HAL_SPI_MspInit
    // I2C pins are enabled in HAL_I2C_MspInit
    // ADC pins are enabled in HAL_ADC_MspInit
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**I2C1 GPIO Configuration
    PB6     ------> I2C1_SCL
    PB7     ------> I2C1_SDA
    */



}