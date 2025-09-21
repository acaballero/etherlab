//
// Created by Angel Dust on 01/03/2020.
//

#include "InputPin.h"
#include "printf.h"
#include "stm32f4xx_hal.h"

uint32_t InputPin::getLastTransitionMs() const {
    return last_transition_ms;
}

uint32_t InputPin::getLastStateChangeMs() const {
    return last_state_change_ms;
}

GPIO_PinState InputPin::getState() {
    return state;
}

void InputPin::handleTransition() {

    // GPIO_PinState s = HAL_GPIO_ReadPin(port, pin);
    uint64_t t = HAL_GetTick();

    // if (state != s) {  // It is really a transition (short spikes may trigger the interrupt)

    if (t - last_transition_ms > debounce_period_ms) {

        // Won't read the state when there's a current pulse, just consider it changes polarity
        // This handles the case of short spikes before real transition which would be read as same value of the current state
        current_transition = state ? GPIO_PIN_RESET : GPIO_PIN_SET;

        // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET);
        // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);
    }

    last_transition_ms = t;
    //}
}

uint32_t InputPin::getLastPeriodMs() const {
    return last_period_ms;
}

void InputPin::reset() {

    last_transition_ms = 0;
    last_state_change_ms = 0;
}

GPIO_PinState InputPin::checkState() {

    uint32_t t = HAL_GetTick();
    bool changed = false;

    GPIO_PinState s = read();

    /*
    if (current_transition) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET);
        for (int i = 0; i < 10000; i++);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);

    }*/

    if (mode == PINMODE_IT && last_transition_ms) {

        // Interrupt mode (just checking for last_transition_ms is enough, as last_transition_ms won't be set in other mode, but we keep it for clarity)

        // If there's a current transition (so we are in debounce phase) and the value has settled after the debounce period (remains the same as in the init of
        // the transition)

        if (current_transition == s && last_transition_ms && (t - last_transition_ms >= debounce_period_ms)) {

            state = s;
            last_period_ms = last_state_change_ms ? t - last_state_change_ms : 0;
            last_state_change_ms = t;
            last_transition_ms = 0;

            changed = true;

            // HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, state);
        } else {
            // printf_("pinmode IT waiting for transition: s: %d, waiting state: %d, ms: %d, p: %d\n", s, current_transition, last_transition_ms,
            //         debounce_period_ms);
        }
    } else { // No interrupt mode or not currently debouncing. Just store it's state and see if it's changed

        if (state != s) {
            last_period_ms = t - last_state_change_ms;
            last_state_change_ms = t;
            changed = true;
        }

        state = s;
    }

    if (changed) {
        //  printf_("Calling input pin %d onchange: state %d\n", getPin(), state);
        if (onChange) {
            onChange();
        }

    } else {
        // printf_("Input pin %d didn't change state %d\n", getPin(), state);
    }

    return state;
    // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);
}

uint16_t InputPin::getDebouncePeriodMs() const {
    return debounce_period_ms;
}

PinMode InputPin::getMode() const {
    return mode;
}
