//
// Created by Angel Dust on 22/01/2021.
//

#ifndef TRX_FRONTEND_IOPIN_H
#define TRX_FRONTEND_IOPIN_H

#include <stm32f4xx.h>

class IOPin {

public:

    IOPin(uint8_t mode) : mode(mode) {};

    virtual GPIO_PinState read() = 0;
    virtual void set(GPIO_PinState) = 0;

protected:

    uint8_t mode;

};

#endif //TRX_FRONTEND_IOPIN_H
