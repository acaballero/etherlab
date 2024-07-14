#ifndef TRX_FRONTEND_POWER_AMP_H
#define TRX_FRONTEND_POWER_AMP_H

#include "Signal.h"

namespace power_amp {

    enum status { OFF, OK, HIGH_TEMP };

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
    extern Signal temp_signal;
    extern Signal status_signal;
    void enable();
    void disable();
    void loop();
}

#endif //TRX_FRONTEND_POWER_AMP_H
