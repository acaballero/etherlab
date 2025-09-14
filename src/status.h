//
// Created by Angel Dust on 06/04/2021.
//

#ifndef TRX_FRONTEND_STATUS_H
#define TRX_FRONTEND_STATUS_H

#include "hw/stm32.h"
#include "hw/hw_config.h"
#include "config.h"
#include "Signal.h"
#include "ui/menu_options.h"
#include "../lib/printf/printf.h"

#if DEBUG_MSGS
#define LOG_NOARGS(with_timestamp, msg)                                                                                                                        \
    {                                                                                                                                                          \
        if (with_timestamp) {                                                                                                                                  \
            printf_("%d: ", HAL_GetTick());                                                                                                                    \
        }                                                                                                                                                      \
        printf_("%s", msg);                                                                                                                                    \
    }
#define LOG_VARS(with_timestamp, msg, ...)                                                                                                                     \
    { ::status::debug_print(msg, with_timestamp, __VA_ARGS__); }
#else
#define LOG_NOARGS(with_timestamp, msg)                                                                                                                        \
    {}
#define LOG_VARS(with_timestamp, msg, ...)                                                                                                                     \
    {}
#endif

// Helper to count arguments and select macro
#define LOG_GET_MACRO(_1, _2, _3, _4, _5, NAME, ...) NAME

// Dispatch macro: handles 1–7 args (add more if needed)
#define LOG(...) LOG_GET_MACRO(__VA_ARGS__, LOG_VARS, LOG_VARS, LOG_VARS, LOG_VARS, LOG_NOARGS)(true, __VA_ARGS__)
#define LOG_RAW(...) LOG_GET_MACRO(__VA_ARGS__, LOG_VARS, LOG_VARS, LOG_VARS, LOG_VARS, LOG_NOARGS)(false, __VA_ARGS__)

namespace status {

extern Signal status_signal;

enum StatusCode { ST_ERROR, ST_WARN, ST_INFO, ST_OK };

typedef struct status_t {
    StatusCode code = ST_OK;
    char msg[30];
} Status;

// Status bar info
struct st_status {
    MODULATION_MODE modulation;
    bool tx;
    radio::BAND band;
    radio::BAND filter;
    radio::IF_FILTER if_filter;
    radio::FRONTEND_PATH frontend_path;
    bool analog_agc;
    bool dsp_agc;
    unsigned long f_carrier;
    Menu::MenuStatus menuStatus = Menu::UNKNOWN;

    bool operator==(const st_status &st) const {
        return modulation == st.modulation && tx == st.tx && frontend_path == st.frontend_path && band == st.band && analog_agc == st.analog_agc &&
               dsp_agc == st.dsp_agc && if_filter == st.if_filter && f_carrier == st.f_carrier && filter == st.filter &&
               menuStatus == st.menuStatus; // or another approach as above
    }
};

extern Status system_status;
void debug_print(const char *str, int timestamp, ...);
void hide_alert();
void pop_alert(StatusCode code, const char *msg);
} // namespace status

#endif // TRX_FRONTEND_STATUS_H
