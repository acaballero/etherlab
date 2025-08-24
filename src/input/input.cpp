//
// Created by Angel Dust on 17/04/2021.
//
#include "hw/stm32.h"
#include "input.h"
#include "hw/hw_config.h"
#include "hw/stm32f4xx/adc.h"
#include "inputEvent.h"
#include "input_controller.h"
#include "../lib/ST77XX-STM32/XPT2046_touch.h"
#include "mcp23017.h"
#include "stm32f4xx_hal_gpio.h"
#include "status.h"

int8_t last_pressed_button_id = -1;
GPIOInputPin FrontPanelInterruptPin(FRONT_PANEL_INTERRUPT_PIN_A, FRONT_PANEL_INTERRUPT_PIN_A_PORT, PINMODE_IT, GPIO_NOPULL, 0, frontPanelInterruptCallback);

GPIOInputPin TouchPanelInterruptPin(TOUCH_IRQ_PIN, TOUCH_IRQ_PORT, PINMODE_IT, GPIO_NOPULL, 2, touchPanelInterruptCallback);

GPIOInputPin BackBtnInputPin(BACK_BTN_PIN, BACK_BTN_GPIO_PORT, PINMODE_IT, GPIO_NOPULL, 0, backBtnInterruptCallback);

// uint16_t analogKeyboardOpenVoltage = 1 << 12; // FULL ADC range by default

void backBtnInterruptCallback() {
    GPIO_PinState state = BackBtnInputPin.getState();

    if (state == GPIO_PIN_SET) { // Just released

        uint32_t period = BackBtnInputPin.getLastPeriodMs();

        // If period==0 it's probably because the button was reset and we should skip this  (for example when pressing and rotating the encoder)
        if (period > 0) {
            input_controller::queue_input_event({INPUT_EVENT_TYPE_BUTTON_PRESS, KEY_BACK, period});
        }
    }
}

void touchPanelInterruptCallback() {
    ;
    xpt2046_touch_check(&xpt2046_touch);
}

uint8_t get_front_panel_int_pin() {
    uint8_t reg = 0;
    mcp23017_read(&hmcp03, REGISTER_INTFA, &reg);
    for (int i = 0; i < 8; i++) {
        if ((reg & (1 << i)) == (1 << i)) {
            return i;
        }
    }
    mcp23017_read(&hmcp03, REGISTER_INTFB, &reg);
    for (int i = 0; i < 8; i++) {
        if ((reg & (1 << i)) == (1 << i)) {
            return i + 8;
        }
    }
    return 16;
}

void frontPanelInterruptCallback() {
    // Note this will be called twice since the interrupt fires also on rising edges
    // and we're using an MCP23017 which makes the interrupt pin go down when the line changes
    // and up when the value is read. On the rising edge, get_front_panel_int_pin wont find a pin value
    // and 'pin' will be set to 16, having no effect
    uint8_t pin = get_front_panel_int_pin();

    if (pin < 16) {
        if (last_pressed_button_id >= 0 && pin == last_pressed_button_id) {
            input_controller::queue_input_event({INPUT_EVENT_TYPE_BUTTON_RELEASE, pin});
            last_pressed_button_id = -1;
        } else {
            input_controller::queue_input_event({INPUT_EVENT_TYPE_BUTTON_PRESS, pin, 0, HAL_GetTick()});
            last_pressed_button_id = pin;
        }

        // LOG("Resetting front panel interrupt: Pin: %d\n", pin);

        uint8_t reg = 0;
        mcp23017_read(&hmcp03, REGISTER_INTCAPA, &reg);
        mcp23017_read(&hmcp03, REGISTER_INTCAPB, &reg);

        HAL_Delay(2); // Let the interrupt pin go up before we check its state again
    }
}

/*void analogKeyboardInterruptCallback() {

    *//*** TODO: As we use the same ADC (ADC1) between FFT captures (in DMA mode) and reading some voltages like this one, there is a chance
         that we try to use the ADC when it's already started in DMA mode. I know this is not the best arrangement,
         and while I redesign the controller board (I ran out of usable analog pins), we have to stop the DMA adquisition and start it again if it's already started.
         Anyway, if we're pushing some keys maybe we don't mind if there's some momentary glich in the FFT
     ***//*

    bool is_fft_adc_started = adc_dma_started;

    if (is_fft_adc_started && ANALOG_KEYBOARD_ADC_HANDLER.Instance == hadc1.Instance) ADC_DMA_Stop(&hadc1);

    uint16_t v = GetADCValue(&ANALOG_KEYBOARD_ADC_HANDLER, ANALOG_KEYBOARD_ADC_CHANNEL, 1);

    if (is_fft_adc_started && ANALOG_KEYBOARD_ADC_HANDLER.Instance == hadc1.Instance) ADC_DMA_Start(&hadc1);

    //GPIO_PinState intState = AnalogKeyBoardInterruptPin.getState();
    uint8_t nbuttons = 4;
    int8_t ibutton = 0;

    float avg = analogKeyboardOpenVoltage / float(nbuttons);  // ADC resolution / number of buttons

    if (v > (nbuttons - 0.5) * avg) { // Open circuit. No button is pressed
        ibutton = 0;
    } else {
        for (int i = 0; i < nbuttons && ibutton == 0; i++) {
            if (v < round((i + 0.5) * avg)) {
                ibutton = i + 1;
            }
        }
    }

    if (ibutton == 0) { // No button pressed -> Released?

        if (analogKeyboardLastPressedButton > 0) {
            onInputEvent({INPUT_EVENT_TYPE_BUTTON_RELEASE, analogKeyboardLastPressedButton});
            analogKeyboardLastPressedButton = 0;
        }
    } else {

        if (analogKeyboardLastPressedButton > 1 && analogKeyboardLastPressedButton == ibutton) {

            //analogKeyboardLastPressedButton = 0; // Second press = exit

        } else {

            onInputEvent({INPUT_EVENT_TYPE_BUTTON_PRESS, ibutton, 0, HAL_GetTick()});
            analogKeyboardLastPressedButton = ibutton;
        }
    }
}*/
