//
// Created by Angel Dust on 06/04/2021.
//

#ifndef TRX_FRONTEND_STATUS_H
#define TRX_FRONTEND_STATUS_H

#include <cstdint>

#include "radio.h"
#include "types.h"
#include "ui/menu_options.h"
#include "../lib/printf/printf.h"

class Signal;

// Avoid pulling the full HAL header here. We only need the tick counter for logging.
extern "C" uint32_t HAL_GetTick(void);

#ifndef DEBUG_MSGS
#define DEBUG_MSGS 1
#endif

#if DEBUG_MSGS
extern int debug_indent;
#define LOG_NOARGS(with_timestamp, increase_indent, msg)                                                                                                       \
    {                                                                                                                                                          \
                                                                                                                                                               \
        if (increase_indent < 0)                                                                                                                               \
            debug_indent += increase_indent;                                                                                                                   \
        if (msg) {                                                                                                                                             \
            if (debug_indent) {                                                                                                                                \
                printf_("%*s", debug_indent, " ");                                                                                                             \
            }                                                                                                                                                  \
            if (with_timestamp) {                                                                                                                              \
                printf_("%d: ", HAL_GetTick());                                                                                                                \
            }                                                                                                                                                  \
            printf_("%s", msg);                                                                                                                                \
        }                                                                                                                                                      \
        if (increase_indent > 0)                                                                                                                               \
            debug_indent += increase_indent;                                                                                                                   \
    }
#define LOG_VARS(with_timestamp, increase_indent, msg, ...)                                                                                                    \
    {                                                                                                                                                          \
        if (increase_indent < 0)                                                                                                                               \
            debug_indent += increase_indent;                                                                                                                   \
        ::status::debug_print(msg, with_timestamp, __VA_ARGS__);                                                                                               \
        if (increase_indent > 0)                                                                                                                               \
            debug_indent += increase_indent;                                                                                                                   \
    }
#else
#define LOG_NOARGS(with_timestamp, increase_indent, msg)                                                                                                       \
    {}
#define LOG_VARS(with_timestamp, increase_indent, msg, ...)                                                                                                    \
    {}
#endif

// Helper to count arguments and select macro
// 1 = msg, _2.._5 = optional format args
#define LOG_GET_MACRO(_1, _2, _3, _4, _5, NAME, ...) NAME

// Dispatch macro: handles 1–7 args (add more if needed)
#define LOG(...) LOG_GET_MACRO(__VA_ARGS__, LOG_VARS, LOG_VARS, LOG_VARS, LOG_VARS, LOG_NOARGS)(true, 0, __VA_ARGS__)
#define LOG_RAW(...) LOG_GET_MACRO(__VA_ARGS__, LOG_VARS, LOG_VARS, LOG_VARS, LOG_VARS, LOG_NOARGS)(false, 0, __VA_ARGS__)

// Calls with specific indentation increase or reduce
#define LOG_IND(indent_delta, ...) LOG_GET_MACRO(__VA_ARGS__, LOG_VARS, LOG_VARS, LOG_VARS, LOG_VARS, LOG_NOARGS)(true, indent_delta, __VA_ARGS__)

// LOG_IND_RAW: without timestamp, explicit indent_delta
#define LOG_IND_RAW(indent_delta, ...) LOG_GET_MACRO(__VA_ARGS__, LOG_VARS, LOG_VARS, LOG_VARS, LOG_VARS, LOG_NOARGS)(false, indent_delta, __VA_ARGS__)

namespace status {

extern Signal status_signal;

enum Level { ERROR, WARN, INFO, OK };
static constexpr uint8_t status_max_length = 30;
typedef struct status_t {
    Level code = OK;
    char msg[status_max_length];
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
void pop_alert(Level code, const char *msg);
} // namespace status

#endif // TRX_FRONTEND_STATUS_H
