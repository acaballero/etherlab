//
// Created by Angel Dust on 01/03/2020.
//

#include "GPIOInputPin.h"

GPIO_PinState GPIOInputPin::read() {

    return HAL_GPIO_ReadPin(this->port, this->pin);
}

uint32_t GPIOInputPin::getPin() const {
    return pin;
}

void GPIOInputPin::setPin(uint32_t pin) {
    this->pin = pin;
}

GPIO_TypeDef *GPIOInputPin::getPort() const {
    return port;
}

void GPIOInputPin::setPort(GPIO_TypeDef *port) {
    this->port = port;
}

/*
 * Initialises the GPIO pin
 */
void GPIOInputPin::init() {

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    /*Configure GPIO pins : ROT_BTN_Pin  */
    GPIO_InitStruct.Pin = this->pin;

    if (this->mode == PINMODE_IT) {

        // In IT mode, the pin must be configured to interrupt at both edges. Otherwise, the current logic won't be able to properly track both states
        GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
    } else {

        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    }

    GPIO_InitStruct.Pull = pull;
    HAL_GPIO_Init(this->port, &GPIO_InitStruct);

    this->state = pull==GPIO_PULLUP ? GPIO_PIN_SET : read();
}

