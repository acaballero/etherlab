//
// Created by Angel Dust on 02/03/2020.
//

#ifndef TRX_FRONTEND_INPUTPINCONTROLLER_H
#define TRX_FRONTEND_INPUTPINCONTROLLER_H

#include "InputPin.h"

#ifdef __cplusplus
extern "C" {
#endif
// Must be defined elsewhere
extern void Error_Handler(void);
#ifdef __cplusplus
}
#endif

#define PINCTRL_MAX_PINS 8

class InputPinController {

  public:
    InputPinController(TIM_TypeDef *timer);

    void addPin(InputPin *pin);

    void handleTimerInterrupt();

    void handlePinEXTI(uint16_t GPIO_Pin);

  private:
    InputPin *pins[PINCTRL_MAX_PINS];
    TIM_HandleTypeDef htim;
    uint8_t npins = 0;

    /*
     * Max debounce period of all the controlled pins. We use it to set the debouncing timer timeout long enough to check for the settling of the slowest pin
     * TODO: The idea here is to use one timer for all the pins and poll only as long as there's one pin who needs to be debounced
     *       Of course this could be further optimized, but we are not considering more than one transition at a time for the moment
     *       I don't know if this class will be used in a scenario of simultaneous transitions (multiple keys at a time?), but that's the idea behind the
     * polling scheme
     */
    uint16_t max_debounce_period = 0;

    /*
     * Las time the timer was started (some time after which no debounce check is needed in any button)
     */
    uint64_t timer_timeout_ms;
};

#endif // TRX_FRONTEND_INPUTPINCONTROLLER_H
