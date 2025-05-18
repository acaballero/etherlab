//
// Created by Angel Dust on 17/04/2021.
//

#include "input_controller.h"
#include "hw/stm32.h"
#include "inputEvent.h"
#include "encoder.h"
#include "input.h"
#include "../ui/menu.h"
#include "config.h"
#include "main_board.h"
#include "stm32f4xx_hal.h"
#include "ui/main_view.h"
#include "settings.h"
#include "status.h"
#include "standby.h"
#include "radio.h"
#include "ui/view_manager.h"
#include "../../lib/ST77XX-STM32/XPT2046_touch.h"

namespace input_controller {
os::periodic_task task(20, dispatchEvents);
}

InputPinController PinController(INPUT_PIN_CONTROLLER_TIMER);

#define SIZEOFINPUTENVENT (sizeof(st_inputEvent))
#if !DISPATCH_INMEDIATELY
st_inputEvent eventQueue[MAX_EVENTS_IN_QUEUE];
st_inputEvent lastEvent;
FIFO inputFIFO{(char *)eventQueue, (MAX_EVENTS_IN_QUEUE * SIZEOFINPUTENVENT)};
#endif

void touch_begin(xpt2046_t *, uint16_t x, uint16_t y) {
    onInputEvent({.type = INPUT_EVENT_TYPE_TOUCH_START, .value = 0, .ms = 0, .time_us = HAL_GetTick(), .point = Point(x, y)});
}

void touch_end(xpt2046_t *, uint16_t x, uint16_t y) {

    st_inputEvent ev{.type = INPUT_EVENT_TYPE_TOUCH_END, .value = 0, .ms = 0, .time_us = HAL_GetTick(), .point = Point(x, y)};

    if (lastEvent.type == INPUT_EVENT_TYPE_TOUCH_START) {
        ev.ms = ev.time_us - lastEvent.time_us;
    }

    onInputEvent(ev);
}

void inputControllerInit() {

    // Initialize touch panel in poll mode
    xpt2046_touch_init(NULL, isoLandscape, 3);
    xpt2046_set_touch_pressed_begin_callback(touch_begin);
    xpt2046_set_touch_pressed_end_callback(touch_end);

    RotAInputPin.init();
    RotBInputPin.init();
    RotBtnInputPin.init();
    BackBtnInputPin.init();
    FrontPanelInterruptPin.init();
    TouchPanelInterruptPin.init();

    // Add pins to controller
    PinController.addPin(&RotBtnInputPin);
    PinController.addPin(&RotAInputPin);
    PinController.addPin(&BackBtnInputPin);
    PinController.addPin(&FrontPanelInterruptPin);
    PinController.addPin(&TouchPanelInterruptPin);

    // calibrateAnalogKeyboard();

    // Set the debounce timer rate
    set_timer_sample_rate(INPUT_PIN_CONTROLLER_TIMER, INPUT_PIN_CONTROLLER_TIMER_CLOCK_HZ, 1000);
}

Widget *processTouch(Widget *w, st_inputEvent *e) {

    if (w->is_point_visible(e->point) && w->enabled()) {

        for (const auto child : w->children()) {
            const auto touched_widget = processTouch(child, e);
            if (touched_widget) {
                return touched_widget;
            }
        }

        // Rect r = w->screen_rect();
        // printf_("Touched: %d,%d %d x %d %d %s\n", r.left(), r.top(), r.right(), r.bottom(), e->type, w->get_name());
        if (w->on_input(*e)) {
            // This widget responded. Return it up the call stack.
            return w;
        }
    }

    return nullptr;
}

void processEvent(st_inputEvent *e) {

    if (e->type == INPUT_EVENT_TYPE_BUTTON_RELEASE || e->type == INPUT_EVENT_TYPE_TOUCH_START || e->type == INPUT_EVENT_TYPE_ENCODER) {
        if (standby::power_mode != standby::POWER_MODE_ON) {
            standby::wakeup();
            return;
        } else if (config.power_save_period_seconds) {
            // resets timeout
            standby::power_save(config.power_save_period_seconds);
        }
    } else if (standby::power_mode != standby::POWER_MODE_ON) {
        return;
    }

    if (!e->time_us) {
        e->time_us = HAL_GetTick();
    }

    if (e->type == INPUT_EVENT_TYPE_BUTTON_PRESS && e->value == lastEvent.value) {

        if (e->time_us) {
            if (e->time_us - lastEvent.time_us < DBL_PRESS_MS) {
                e->type = INPUT_EVENT_TYPE_BUTTON_DBL_PRESS;
            }
        }
    }

    if (e->type == INPUT_EVENT_TYPE_TOUCH_START || e->type == INPUT_EVENT_TYPE_TOUCH_END) {
        Widget *w = processTouch((Widget *)view_manager::currentView, e);
        if (e->type == INPUT_EVENT_TYPE_TOUCH_START && w) {
            // Force a paint here to draw whatever change is required in the widget. Not pretty but since
            // the regular paint occurs at fixed intervals, a button press may, for example, miss the style change
            // between touch start and end.
            w->paint();
        }
    } else if (!view_manager::currentView->on_input(*e)) {

        // TODO: Consume these events in their appropriate views/widget

        switch (e->type) {

            case INPUT_EVENT_TYPE_BUTTON_PRESS:
            case INPUT_EVENT_TYPE_BUTTON_DBL_PRESS:

                switch (e->value) {

                    case FPANEL_PAD_BUTTON_1: // PTT
                        MODE mode;
                        if (config.mode == ANALOG_RX || config.mode == ANALOG_TX) {
                            mode = config.mode == ANALOG_RX ? ANALOG_TX : ANALOG_RX;
                        } else {
                            mode = config.mode == DIGITAL_RX ? DIGITAL_TX : DIGITAL_RX;
                        }

                        main_board::setMode(mode);
                        break;

                    case BTN_ENCODER:

                        bool very_long_press = e->ms > VERY_LONG_PRESS_MS;

                        if (very_long_press) {
                            if (settings_write(&config) == HAL_FLASH_ERROR_NONE) {
                                status::handleError(status::ST_INFO, "Configuration saved");
                            } else {
                                status::handleError(status::ST_ERROR, "Error saving configuration");
                            }
                        }
                        break;
                }

                break;

            case INPUT_EVENT_TYPE_BUTTON_RELEASE:

                switch (e->value) {
                    case FPANEL_PAD_BUTTON_1: // Release TX
                        if (lastEvent.type != INPUT_EVENT_TYPE_BUTTON_DBL_PRESS || lastEvent.value != e->value) {
                            main_board::setMode(config.mode == DIGITAL_TX ? DIGITAL_RX : ANALOG_RX);
                        }
                        break;
                }

                break;

            case INPUT_EVENT_TYPE_ENCODER:

                if (Menu::menuStatus != Menu::ACTIVE) {
                    if (RotBtnInputPin.getState() == GPIO_PIN_RESET) { // with push button low, change the step size instead of frequency
                        RotBtnInputPin.reset();

                        radio::change_step(-e->value);
                    } else {

                        radio::change_frequency(e->value);
                    }
                }
                break;
            default:
                break;
        }
    }

    memcpy(&lastEvent, e, sizeof(st_inputEvent));
}

void onInputEvent(st_inputEvent e) {

#if DISPATCH_INMEDIATELY
    processEvent(&e);
#else
    inputFIFO.writeBlock((char *)&e, SIZEOFINPUTENVENT);
#endif
}

void dispatchEvents() {

#if !DISPATCH_INMEDIATELY
    st_inputEvent *e;

    while (inputFIFO.available((char **)&e) >= SIZEOFINPUTENVENT) {
        processEvent(e);
        inputFIFO.consume(SIZEOFINPUTENVENT, (char **)&e);
    }
#endif
}

/* Handler for the Timer #13. It's sent to the InputPinController for debouncing purposes
 * TODO: Make it variable depending on the board we're building the project for */
void TIM8_UP_TIM13_IRQHandler(void) {
    // GPIOE->BSRR |= GPIO_PIN_13;
    PinController.handleTimerInterrupt();
    // GPIOE->BSRR |= GPIO_PIN_13 << 16;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    // TODO: Receiving all callbacks in this handler is ambiguous since the port is missing
    // GPIOE->BSRR |= GPIO_PIN_13;

    PinController.handlePinEXTI(GPIO_Pin);

    // GPIOE->BSRR |= GPIO_PIN_13 << 16;
}
