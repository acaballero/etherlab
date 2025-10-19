//
// Created by Angel Dust on 06/04/2021.
//

#include "status.h"
#include "Signal.h"
#include "hw/hw_config.h"
#include "stm32f4xx_hal.h"
#include <printf.h>

namespace status {

Status system_status;
Signal status_signal;

void debug_print(const char *str, int timestamp, ...) {
    va_list argptr;
    va_start(argptr, timestamp);
    auto t = HAL_GetTick();
#if SWO_ENABLED
    if (timestamp) {
        printf_("%d: ", t);
    }
    vprintf_(str, argptr);
#elif USB_PRINT_ENABLED
    if (timestamp) {
        usb.print("%d: ", t);
    }
    usb.print(str, argptr);
#endif
    va_end(argptr);
}

void hide_alert() {
    system_status.code = OK;
    status_signal.emit(&system_status);
}

void pop_alert(Level code, const char *msg) {

    LOG("%s: %s\n", code == WARN ? "WARNING" : ERROR ? "ERROR" : "INFO", msg) // Print to console, if enabled

    system_status.code = code;
    snprintf(system_status.msg, 40, "%s", msg);

    status_signal.emit(&system_status);
}
} // namespace status
