#ifndef TRX_FRONTEND_POWER_AMP_H
#define TRX_FRONTEND_POWER_AMP_H

#include "Signal.h"
#include "os/periodic_task.h"
#include <sys/_stdint.h>

namespace power_amp {

enum status { OFF, OK, HIGH_TEMP, SHUTDOWN };

// Temperature is relative to the thermistor
struct st_power_amp_params {
    const float MIN_TEMP = 0;
    const float TEMP_LOW_THRESHOLD = 44.0;
    const float TEMP_HIGH_THRESHOLD = 50.0;
    const float MAX_TEMP = 80;
};

extern status status;
extern st_power_amp_params params;
extern int temp;
extern uint64_t last_hpa_shutdown_ms;
extern uint32_t hpa_shutdown_timeout_ms;
extern Signal temp_signal;
extern Signal status_signal;
extern os::periodic_task task;
void enable();
void disable();
void shutdown();

} // namespace power_amp

#endif // TRX_FRONTEND_POWER_AMP_H
