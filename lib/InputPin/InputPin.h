//
// Created by Angel Dust on 01/03/2020.
//

#ifndef TRX_FRONTEND_INPUTPIN_H
#define TRX_FRONTEND_INPUTPIN_H

#include <stm32f4xx.h>

enum PinMode {
    PINMODE_IT,
    PINMODE_POLL
};


class InputPin {

public:

    InputPin(PinMode mode, uint16_t debounce_ms, void (*onChange)()) : debounce_period_ms(debounce_ms),mode(mode),onChange(onChange) {};

    virtual void init() = 0;
    virtual GPIO_PinState read() = 0;
    virtual uint32_t getPin() const = 0;

    GPIO_PinState getState();

    GPIO_PinState checkState();

    uint32_t getLastTransitionMs() const;

    uint32_t getLastStateChangeMs() const;

    uint32_t getLastPeriodMs() const;

    void handleTransition();

    uint16_t getDebouncePeriodMs() const;

    PinMode getMode() const;

    void reset();

protected:

    uint16_t debounce_period_ms;
    PinMode mode;
    GPIO_PinState state;

private:



    void (*onChange)();

    GPIO_PinState current_transition;
    uint32_t last_transition_ms;
    uint32_t last_state_change_ms;
    uint32_t last_period_ms;

};
#endif //TRX_FRONTEND_INPUTPIN_H
