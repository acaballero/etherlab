//
// Created by Angel Dust on 06/04/2021.
//

#ifndef TRX_FRONTEND_STATUS_H
#define TRX_FRONTEND_STATUS_H

#include "hw/stm32.h"
#include "hw/hw_config.h"
#include "config.h"
#include "Signal.h"

#if DEBUG_MSGS
#if SWO_ENABLED
#define DEBUGPRINT(msg,...) {status::debug_print(msg,__VA_ARGS__);}
#else
#define DEBUGPRINT(msg,...) {}
#endif
#endif

namespace status {

    extern Signal status_signal;

    enum StatusCode {
        ST_ERROR, ST_WARN, ST_INFO, ST_OK
    };

    typedef struct {
        StatusCode code;
        char msg[30];
    } Status;

    extern Status systemStatus;
    inline void debug_print(const char *str, ...);
    void clearError();
    void handleError(StatusCode code, const char *msg);
}

#endif //TRX_FRONTEND_STATUS_H
