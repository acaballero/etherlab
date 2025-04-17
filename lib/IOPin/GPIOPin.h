//
// Created by Angel Dust on 22/01/2021.
//

#ifndef TRX_FRONTEND_GPIOPIN_H
#define TRX_FRONTEND_GPIOPIN_H

#include <stm32f4xx.h>
#include "IOPin.h"

class GPIOPin : public IOPin {

  public:
    GPIOPin(uint16_t pin, GPIO_TypeDef *port, uint8_t mode) : IOPin(mode), pin(pin), port(port){};
    GPIO_PinState read() override;
    GPIO_PinState toggle() override;
    uint8_t set(GPIO_PinState) override;

  private:
    uint16_t pin;
    GPIO_TypeDef *port;
};

#endif // TRX_FRONTEND_GPIOPIN_H
