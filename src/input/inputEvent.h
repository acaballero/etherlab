//
// Created by Angel Dust on 17/04/2021.
//

#ifndef TRX_FRONTEND_INPUTEVENT_H
#define TRX_FRONTEND_INPUTEVENT_H

#include <stdio.h>
#include <stddef.h>
#include "ui/ui_types.h"

enum BUTTON_ID {
        FPANEL_PAD_BUTTON_1 = 0,
        FPANEL_PAD_BUTTON_2 = 1,
        FPANEL_PAD_BUTTON_3 = 2,
        FPANEL_PAD_BUTTON_4 = 3,
        BTN_ENCODER = 3,
        FPANEL_PAD_BUTTON_5 = 4,
        KEY_BACK = 4,
        FPANEL_PAD_BUTTON_6 = 5,
        FPANEL_DISPLAY_BUTTON_6 = 8,
        FPANEL_DISPLAY_BUTTON_5 = 9,
        FPANEL_DISPLAY_BUTTON_4 = 10,
        FPANEL_DISPLAY_BUTTON_3 = 11,
        FPANEL_DISPLAY_BUTTON_2 = 12,
        FPANEL_DISPLAY_BUTTON_1 = 13
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
        uint32_t ms{0};      // Milliseconds since last transition
        uint64_t time_us{0}; // Timestamp (microseconds)
        Point point;
};

#endif // TRX_FRONTEND_INPUTEVENT_H
