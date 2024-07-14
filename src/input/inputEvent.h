//
// Created by Angel Dust on 17/04/2021.
//

#ifndef TRX_FRONTEND_INPUTEVENT_H
#define TRX_FRONTEND_INPUTEVENT_H

#include <stdio.h>
#include <stddef.h>
#include "ui/ui_types.h"

enum BUTTON_ID {
    KEY_1 = 1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_BACK = 100,
    BTN_ENCODER = 1000
};

#define DBL_PRESS_MS 400

enum INPUT_EVENT_TYPE {
    INPUT_EVENT_TYPE_ENCODER,
    INPUT_EVENT_TYPE_BUTTON_PRESS,
    INPUT_EVENT_TYPE_BUTTON_DBL_PRESS,
    INPUT_EVENT_TYPE_BUTTON_RELEASE,
    INPUT_EVENT_TYPE_TOUCH_START,
    INPUT_EVENT_TYPE_TOUCH_END,
    INPUT_EVENT_TYPE_NONE
};

struct st_inputEvent {
    INPUT_EVENT_TYPE type{INPUT_EVENT_TYPE_NONE};
    int value{0};
    uint32_t ms{0}; // Milliseconds since last transition
    uint64_t time_us{0}; // Timestamp (microseconds)
    Point point;
};

#endif //TRX_FRONTEND_INPUTEVENT_H
