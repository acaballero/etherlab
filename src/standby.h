//
// Created by Angel Dust on 21/06/2024.
//

#ifndef TRX_FRONTEND_STANDBY_H
#define TRX_FRONTEND_STANDBY_H

#include "Signal.h"

namespace standby {

    enum POWER_MODE {
        POWER_MODE_ON, POWER_MODE_SLEEP
    };

    extern POWER_MODE power_mode;
    extern Signal signal;

    int sleep();
    int wakeup();
}

#endif //TRX_FRONTEND_STANDBY_H
