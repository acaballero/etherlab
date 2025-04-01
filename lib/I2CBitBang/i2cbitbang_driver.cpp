/*
 * i2cbitbang_driver.cpp
 *
 *  Created on: 25 lis 2019
 *      Author: Tata
 */

#include "i2cbitbang.h"

/*
 * BE CAREFUL!
 * It supports only pins which are GPIO as non-alternate function
 *
 */

#define I2CBITBANG_SCL_PORT GPIOB
#define I2CBITBANG_SCL_PIN GPIO_PIN_6
#define I2CBITBANG_SDA_PORT GPIOB
#define I2CBITBANG_SDA_PIN GPIO_PIN_7

const i2cbbConfig_t i2cbbConfigTable[NUMBER_OF_I2CBB_INSTANCES] = {
    {I2CBITBANG_SCL_PORT, I2CBITBANG_SCL_PIN,  /*SCL 0*/
     I2CBITBANG_SDA_PORT, I2CBITBANG_SDA_PIN}, /*SDA 0*/
};

void i2cbitbang::conf_hardware(void) {
    GPIO_InitTypeDef GPIO_InitStruct;

    if (i2cbbConfigTable[instance].GPIOclk_port == GPIOA || i2cbbConfigTable[instance].GPIOdta_port == GPIOA) {
        do {
            volatile uint32_t tmpreg = 0x00U;
            ((((RCC_TypeDef *)((0x40000000UL + 0x00020000UL) + 0x3800UL))->AHB1ENR) |= ((0x1UL << (0U))));
            tmpreg = ((((RCC_TypeDef *)((0x40000000UL + 0x00020000UL) + 0x3800UL))->AHB1ENR) & ((0x1UL << (0U))));
            (void)tmpreg;
        } while (0U);
    }
    if (i2cbbConfigTable[instance].GPIOclk_port == GPIOB || i2cbbConfigTable[instance].GPIOdta_port == GPIOB) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
    }
    if (i2cbbConfigTable[instance].GPIOclk_port == GPIOC || i2cbbConfigTable[instance].GPIOdta_port == GPIOC) {
        __HAL_RCC_GPIOC_CLK_ENABLE();
    }
    if (i2cbbConfigTable[instance].GPIOclk_port == GPIOD || i2cbbConfigTable[instance].GPIOdta_port == GPIOD) {
        __HAL_RCC_GPIOD_CLK_ENABLE();
    }
    if (i2cbbConfigTable[instance].GPIOclk_port == GPIOE || i2cbbConfigTable[instance].GPIOdta_port == GPIOE) {
        __HAL_RCC_GPIOE_CLK_ENABLE();
    }
    if (i2cbbConfigTable[instance].GPIOclk_port == GPIOF || i2cbbConfigTable[instance].GPIOdta_port == GPIOF) {
        __HAL_RCC_GPIOF_CLK_ENABLE();
    }

    HAL_GPIO_WritePin(i2cbbConfigTable[instance].GPIOdta_port, i2cbbConfigTable[instance].pinDta, GPIO_PIN_SET);
    HAL_GPIO_WritePin(i2cbbConfigTable[instance].GPIOclk_port, i2cbbConfigTable[instance].pinClk, GPIO_PIN_SET);

    /*Configure GPIO clk */

    GPIO_InitStruct.Pin = i2cbbConfigTable[instance].pinClk;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(i2cbbConfigTable[instance].GPIOclk_port, &GPIO_InitStruct);

    /*Configure GPIO dta */
    GPIO_InitStruct.Pin = i2cbbConfigTable[instance].pinDta;
    HAL_GPIO_Init(i2cbbConfigTable[instance].GPIOdta_port, &GPIO_InitStruct);
}

void i2cbitbang::i2c_bus_init(void) {
    error = I2CBB_ERROR_NONE;
    set_SDA();
    set_SCL();

    // TODO confirm that SDA and SCL are set, otherwise bus error
}

bool i2cbitbang::read_SCL(void) // Return current level of SCL line, 0 or 1
{
    if (HAL_GPIO_ReadPin(i2cbbConfigTable[instance].GPIOclk_port, i2cbbConfigTable[instance].pinClk) == GPIO_PIN_SET) {
        return 1;
    } else {
        return 0;
    }
}

bool i2cbitbang::read_SDA(void) // Return current level of SDA line, 0 or 1
{

    if (HAL_GPIO_ReadPin(i2cbbConfigTable[instance].GPIOdta_port, i2cbbConfigTable[instance].pinDta) == GPIO_PIN_SET) {
        return 1;
    } else {
        return 0;
    }
}

void i2cbitbang::set_SCL(void) // Do not drive SCL (set pin high-impedance)
{
    //	conf_SCL(I2CBB_INTPUT);
    HAL_GPIO_WritePin(i2cbbConfigTable[instance].GPIOclk_port, i2cbbConfigTable[instance].pinClk, GPIO_PIN_SET);
}

void i2cbitbang::clear_SCL(void) // Actively drive SCL signal low
{
    HAL_GPIO_WritePin(i2cbbConfigTable[instance].GPIOclk_port, i2cbbConfigTable[instance].pinClk, GPIO_PIN_RESET);
}

void i2cbitbang::set_SDA(void) // Do not drive SDA (set pin high-impedance)
{
    HAL_GPIO_WritePin(i2cbbConfigTable[instance].GPIOdta_port, i2cbbConfigTable[instance].pinDta, GPIO_PIN_SET);
}

void i2cbitbang::clear_SDA(void) // Actively drive SDA signal low
{
    HAL_GPIO_WritePin(i2cbbConfigTable[instance].GPIOdta_port, i2cbbConfigTable[instance].pinDta, GPIO_PIN_RESET);
}
