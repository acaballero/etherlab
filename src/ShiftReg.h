//
// Created by Angel Dust on 29/02/2020.
// A 'bit banged' Shift register controller
// All three control lines are controlled with GPIO pins

#ifndef TRX_FRONTEND_SHIFTREG_H
#define TRX_FRONTEND_SHIFTREG_H

#include "hw/stm32.h"
#include "../lib/IOPin/IOPin.h"

class ShiftReg {

public:

    ShiftReg(IOPin *data_pin,IOPin *clk_pin, IOPin *set_pin);
    void write(uint16_t word,uint8_t size);
    void write(uint8_t byte);
    uint8_t getValue() const;

private:

    IOPin *data_pin,*clk_pin,*set_pin;
    uint16_t value;

};

#endif //TRX_FRONTEND_SHIFTREG_H
