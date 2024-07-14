//
// Created by Angel Dust on 01/03/2020.
//

#ifndef TRX_FRONTEND_MCP23017INPUTPIN_H
#define TRX_FRONTEND_MCP23017INPUTPIN_H

#include <stm32f4xx.h>
#include "../MCP23017/mcp23017.h"
#include "InputPin.h"

class MCP23017InputPin : public InputPin {

public:

    MCP23017InputPin(uint32_t pin,uint8_t port,MCP23017_HandleTypeDef *handle,PinMode mode, uint16_t debounce_ms, void (*onChange)()) : InputPin(mode,debounce_ms,onChange),pin(pin),port(port),handle(handle) {};

    uint32_t getPin() const;

    void setPin(uint32_t pin);

    void init();

    GPIO_PinState read();



private:


    uint32_t pin;
    uint8_t port;
    MCP23017_HandleTypeDef * handle;


};
#endif //TRX_FRONTEND_MCP23017INPUTPIN_H
