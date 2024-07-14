//
// Created by Angel Dust on 02/03/2020.
//

#include "InputPinController.h"
#include "../../lib/utils/utils.hpp"

InputPinController::InputPinController(TIM_TypeDef *timer) {


    // Initialization of the timer used to check for the settling of the pin value once a transition has been detected

    TIM_MasterConfigTypeDef sMasterConfig = {0};
    //TIM_OC_InitTypeDef sConfigOC = {0};

    htim.Instance = timer;
    htim.Init.Prescaler = 7200;
    htim.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim.Init.Period = 1000;
    htim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_Base_Init(&htim) != HAL_OK) {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }
    /* sConfigOC.OCMode = TIM_OCMODE_TIMING;
     sConfigOC.Pulse = 0;
     sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
     sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
     if (HAL_TIM_OC_ConfigChannel(&htim, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
         Error_Handler();
     }*/

    HAL_TIM_Base_Start_IT(&htim);
}

void InputPinController::addPin(InputPin *pin) {

    this->pins[this->npins++] = pin;
    this->max_debounce_period = max2(this->max_debounce_period, pin->getDebouncePeriodMs());
}

void InputPinController::handleTimerInterrupt() {

    // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET);
    // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET);
    for (int i = 0; i < this->npins; i++) {
        this->pins[i]->checkState();
    }

    uint64_t t = HAL_GetTick();

    // Let HAL do it's shit with the timer
    HAL_TIM_IRQHandler(&htim);

    // If the debouncing timer has been on for two debouncing periods from the last time a pin changed its value, stop it
    if (t - this->timer_timeout_ms > this->max_debounce_period << 1) {
        HAL_TIM_Base_Stop_IT(&htim);
    }
}

/*
 * Handles the external interrupts of the GPIO pins. It should be called externally from the real interrupt handler
 */
void InputPinController::handlePinEXTI(uint16_t GPIO_Pin) {

    InputPin *pin = NULL;

    // Search for the pin who fired the interrupt
    for (int i = 0; i < this->npins && pin == NULL; i++) {

        if (this->pins[i]->getPin() == GPIO_Pin) {

            pin = this->pins[i];
        }
    }

    if (pin) {

        // Let the pin handle the transition
        pin->handleTransition();

        // If the button needs debouncing, start or extend the debounce timer
        if (pin->getDebouncePeriodMs()) {

            uint64_t t = HAL_GetTick();
            // Extend/start the timer and set the timeout to stop polling after the debounce period
            if (!htim.Instance->CCER) {

                HAL_TIM_Base_Start_IT(&htim);

            }

            this->timer_timeout_ms = t;
        } else {
            // If the pin doesn't need debouncing, just check it's state
            pin->checkState();
        }
    }
}
