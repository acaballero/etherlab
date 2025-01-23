//
// Created by Angel Dust on 22/01/2021.
//

#include "GPIOPin.h"

GPIO_PinState GPIOPin::read() { return HAL_GPIO_ReadPin(this->port, this->pin); }

GPIO_PinState GPIOPin::toggle() {
    GPIO_PinState state = HAL_GPIO_ReadPin(this->port, this->pin);
    GPIO_PinState new_state = state == GPIO_PIN_SET ? GPIO_PIN_RESET : GPIO_PIN_SET;
    set(new_state);
    return new_state;
}

void GPIOPin::set(GPIO_PinState state) { HAL_GPIO_WritePin(this->port, this->pin, state); }
