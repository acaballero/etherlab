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

#if DEBUG_MSGS
#if SWO_ENABLED
#define DEBUGPRINT(msg, ...)                                                                                                                                   \
    { status::debug_print(msg, __VA_ARGS__); }
#else
#define DEBUGPRINT(msg, ...)                                                                                                                                   \
    {}
#endif
#endif

namespace status {

extern Signal status_signal;

enum StatusCode { ST_ERROR, ST_WARN, ST_INFO, ST_OK };

typedef struct {
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
    bool agc;
    unsigned long f_carrier;
    Menu::MenuStatus menuStatus = Menu::UNKNOWN;

    bool operator==(const st_status &st) const {
        return modulation == st.modulation && tx == st.tx && frontend_path == st.frontend_path && band == st.band && agc == st.agc &&
               if_filter == st.if_filter && f_carrier == st.f_carrier && filter == st.filter && menuStatus == st.menuStatus; // or another approach as above
    }
};

extern Status systemStatus;
inline void debug_print(const char *str, ...);
void clearError();
void handleError(StatusCode code, const char *msg);
} // namespace status

#endif // TRX_FRONTEND_STATUS_H
