//
// Created by Angel Dust on 29/02/2020.
//

#include "ShiftReg.h"
#include "stm32f4xx_hal_gpio.h"

ShiftReg::ShiftReg(IOPin *data_pin, IOPin *clk_pin, IOPin *set_pin) {

    this->data_pin = data_pin;
    this->clk_pin = clk_pin;
    this->set_pin = set_pin;
}

void ShiftReg::write(uint8_t byte) {
    this->write(byte, 8);
}

/*
 * Writes 'size' bits to the shift register
 */
void ShiftReg::write(uint16_t word, uint8_t size) {

    this->value = word;

    uint16_t mask = (1 << (size - 1));

    for (int i = 0; i < size; i++) {

        data_pin->set((GPIO_PinState)(bool)(word & mask));
        clk_pin->set(GPIO_PIN_SET);
        clk_pin->set(GPIO_PIN_RESET);

        word <<= 1;
    }

    // Latch byte to the output flip-flop
    set_pin->set(GPIO_PIN_SET);
    set_pin->set(GPIO_PIN_RESET);
}

uint8_t ShiftReg::getValue() const {
    return this->value;
}
