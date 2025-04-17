//
// Created by Angel Dust on 22/01/2021.
//

#ifndef TRX_FRONTEND_MCP23017PIN_H
#define TRX_FRONTEND_MCP23017PIN_H

#include <stm32f4xx.h>
#include "../MCP23017/mcp23017.h"
#include "IOPin.h"

class MCP23017Pin : public IOPin {

  public:
    MCP23017Pin(uint16_t pin, uint8_t port, MCP23017_HandleTypeDef *handle, uint8_t mode)
        : IOPin(mode), pin(pin), port(port), handle(handle){

                                             };
    GPIO_PinState read() override;
    GPIO_PinState toggle() override;
    uint8_t set(GPIO_PinState) override;

  private:
    uint16_t pin;
    uint8_t port;
    MCP23017_HandleTypeDef *handle;
};

#endif // TRX_FRONTEND_MCP23017PIN_H
