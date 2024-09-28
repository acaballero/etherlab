//
// Created by Angel Dust on 01/03/2020.
//

#ifndef TRX_FRONTEND_GPIOINPUTPIN_H
#define TRX_FRONTEND_GPIOINPUTPIN_H

#include "InputPin.h"
#include <stm32f4xx.h>

class GPIOInputPin : public InputPin {

public:

    // The minimum debounce period is determined by the timer period, which, at the time of writing this, is 5ms
    GPIOInputPin(uint32_t pin,GPIO_TypeDef *port,PinMode mode, uint16_t debounce_ms, void (*onChange)()) : GPIOInputPin(pin,port,mode,GPIO_PULLUP,debounce_ms,onChange) {};
    GPIOInputPin(uint32_t pin,GPIO_TypeDef *port,PinMode mode, uint32_t pull, uint16_t debounce_ms, void (*onChange)()) : InputPin(mode,debounce_ms,onChange), pin(pin),port(port), pull(pull) {};
    GPIOInputPin(uint32_t pin,GPIO_TypeDef *port,InterruptMode mode, uint32_t pull, uint16_t debounce_ms, void (*onChange)()) : InputPin(mode,debounce_ms,onChange), pin(pin),port(port), pull(pull) {};
    void init();

    GPIO_PinState read();

    uint32_t getPin() const;

    void setPin(uint32_t pin);

    GPIO_TypeDef *getPort() const;

    void setPort(GPIO_TypeDef *port);

private:

    uint32_t pin;
    GPIO_TypeDef * port;
    uint32_t pull;

};
#endif //TRX_FRONTEND_GPIOINPUTPIN_H
