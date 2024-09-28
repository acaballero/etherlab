//
// Created by Angel Dust on 01/03/2020.
//
// TODO: Interrupts are not tested

#include "MCP23017InputPin.h"

uint32_t MCP23017InputPin::getPin() const {
    return pin;
}

void MCP23017InputPin::setPin(uint32_t p) {
    this->pin = p;
}

GPIO_PinState MCP23017InputPin::read() {

    mcp23017_read_gpio(this->handle, this->port);
    return (GPIO_PinState)(this->handle->gpio[this->port] & (1 << pin));

}

/*
 * Initialises the GPIO pin
 */
void MCP23017InputPin::init() {

    uint8_t data;

    if (this->mode==PINMODE_IT) {

        // Configure interrupt
        mcp23017_read (this->handle,REGISTER_IPOLA | this->port,&data);
        mcp23017_writereg (this->handle,REGISTER_IPOLA | this->port, data | (1<<this->pin)); // invert polarity (buttons are pulled up, so interrupts occur when they go low)

        mcp23017_read (this->handle,REGISTER_GPINTENA | this->port,&data);
        mcp23017_writereg (this->handle,REGISTER_GPINTENA | this->port, data | (1<<this->pin));

        // In IT mode, the pin must be configured to interrupt at both edges. Otherwise, the current logic won't be able to properly track both states
    }

    // Set input pull-up
    mcp23017_read (this->handle,REGISTER_GPPUA | this->port,&data);
    mcp23017_ggpu(this->handle, this->port, data | (1<<this->pin));

    this->state=GPIO_PIN_SET; // Pull-up
}

