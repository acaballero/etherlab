//
// Created by Angel Dust on 06/04/2021.
//

#include "status.h"
#include "Signal.h"
#include "hw/hw_config.h"
#include <printf.h>

namespace status {

Status systemStatus;
Signal status_signal;

inline void debug_print(const char *str, ...) {
    va_list argptr;
    va_start(argptr, str);
#if SWO_ENABLED
    printf(str, argptr);
#elif USB_PRINT_ENABLED
    usb.print(str, argptr);
#endif
    va_end(argptr);
}

void clearError() {
    systemStatus.code = ST_OK;
    status_signal.emit(&systemStatus);
}

void handleError(StatusCode code, const char *msg) {

    DEBUGPRINT(msg, 0)  // Print to console, if enabled
    DEBUGPRINT("\n", 0) // Print to console, if enabled

    systemStatus.code = code;
    snprintf(systemStatus.msg, 30, "%s", msg);

    status_signal.emit(&systemStatus);
}
} // namespace status
