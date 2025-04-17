//
// Created by Angel Dust on 22/01/2021.
//

#include "MCP23017Pin.h"
#include "../printf/printf.h"

GPIO_PinState MCP23017Pin::read() {

    mcp23017_read_gpio(this->handle, this->port);
    return (GPIO_PinState)(this->handle->gpio[this->port] & (1 << this->pin));
}

uint8_t MCP23017Pin::set(GPIO_PinState state) {

    uint8_t curr_state = this->handle->gpio[this->port];

    if (state) {
        this->handle->gpio[this->port] |= (1 << pin);
    } else {
        this->handle->gpio[this->port] &= ~(1 << pin);
    }

    if (curr_state != this->handle->gpio[this->port]) {
        return mcp23017_write_gpio(this->handle, this->port);
    }

    return 0;
}

GPIO_PinState MCP23017Pin::toggle() {
    GPIO_PinState state = read();
    GPIO_PinState new_state = state == GPIO_PIN_SET ? GPIO_PIN_RESET : GPIO_PIN_SET;
    set(new_state);
    return new_state;
}
