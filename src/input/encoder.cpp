//
// Created by Angel Dust on 17/04/2021.
//

#include "encoder.h"

#include "hw/stm32.h"
#include "input.h"
#include "inputEvent.h"
#include "input_controller.h"
#include "../../lib/InputPin/GPIOInputPin.h"
#include "../../lib/InputPin/MCP23017InputPin.h"
#include "hw/hw_config.h"
#include "../../lib/utils/utils.hpp"

unsigned long rot_last_click_ms = 0;
uint8_t rot_max_clicks_rate = 90;
uint8_t rot_min_clicks_rate = 20;
int rot_rate = 0;
int direction;

GPIOInputPin RotAInputPin(ROT_A_PIN, ROT_A_GPIO_PORT, PINMODE_IT, 0, doEncoderA); // No debounce needed with the decoupling cap added in this pin
GPIOInputPin RotBtnInputPin(ROT_BTN_PIN, ROT_BTN_GPIO_PORT, PINMODE_IT, 0, doPushButton);
GPIOInputPin RotBInputPin(ROT_B_PIN, ROT_B_GPIO_PORT, PINMODE_POLL, 0, NULL);

void doEncoderA() {

    RotBtnInputPin.reset(); // disable long press in case the button was down

    uint8_t sA = RotAInputPin.getState();
    uint8_t sB = RotBInputPin.checkState();

    if (!sA) { // Only consider falling edges of input A

        direction = sB > 0 ? -1 : 1; // The direction depends on the state of the other input

        unsigned long m = HAL_GetTick();

        if (rot_last_click_ms > 0) {

            int rate = max2(1, int(1000.0 / (float)(m - rot_last_click_ms))); // clicks per second

            rate = constrain(rate, rot_min_clicks_rate, rot_max_clicks_rate) - rot_min_clicks_rate;

            // exponential smoothing
            rot_rate = (int)ceil((float)rot_rate - (0.95f * (float)(rot_rate - rate)));

            double max_rate = rot_max_clicks_rate - rot_min_clicks_rate;

            // cubic acceleration for comfortable use
            double speed = pow(rot_rate, 3) / pow(max_rate, 3);

            int step_lin = (int)(speed * (float)(ROTARY_ENCODER_mAX_STEP - ROTARY_ENCODER_mIN_STEP)) + ROTARY_ENCODER_mIN_STEP;

            // round to log
            int step = (int)ceil(pow(10, floor(log10(step_lin))));

            direction *= step;

            /*
            serial.print("rate:");
            serial.print(rate);
            serial.print("rot rate:");
            serial.print(rot_rate);
            serial.print(" speed:");
            serial.print(speed);
            serial.print(" step:");
            serial.println(step);
            serial.print(" step_lin:");
            serial.println(step_lin);
             */
        }

        rot_last_click_ms = m;

        input_controller::queue_input_event({INPUT_EVENT_TYPE_ENCODER, direction});
    }
}

void doPushButton() {

    GPIO_PinState state = RotBtnInputPin.getState();

    if (state == GPIO_PIN_SET) { // Just released

        uint32_t period = RotBtnInputPin.getLastPeriodMs();

        // If period==0 it's probably because the button was reset and we should skip this  (for example when pressing and rotating the encoder)
        if (period > 0) {
            input_controller::queue_input_event({INPUT_EVENT_TYPE_BUTTON_PRESS, BTN_ENCODER, period});
        }
    }
}
