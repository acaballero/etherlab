//
// Created by Angel Dust on 08/01/2026.
//

#ifndef __APRS_SETTINGS_H__
#define __APRS_SETTINGS_H__

#include "stdint.h"

namespace aprs {

struct settings {
    uint16_t beacon_period_ms{10000};
    uint16_t deviation{3500};
    char path[32]{"WIDE1-1,WIDE2-2"};
};
} // namespace aprs
#endif
