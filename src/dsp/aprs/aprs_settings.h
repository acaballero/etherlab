//
// Created by Angel Dust on 08/01/2026.
//

#ifndef __APRS_SETTINGS_H__
#define __APRS_SETTINGS_H__

#include "stdint.h"
#include <cstring>

namespace aprs {

static constexpr int max_message_length = 128;
static constexpr int max_path_length = 32;

struct settings {
    uint16_t beacon_period_ms{10000};
    uint16_t deviation{3500};
    char path[max_path_length]{"WIDE1-1,WIDE2-2"};
    char message[max_message_length]{"!4045.22N / 00347.24W - Angel Dust QTH "};

    settings &operator=(const settings &other) {
        if (this != &other) {
            beacon_period_ms = other.beacon_period_ms;
            deviation = other.deviation;
            strncpy(path, other.path, max_path_length - 1);
            path[max_path_length - 1] = '\0';
            strncpy(message, other.message, max_message_length - 1);
            message[max_message_length - 1] = '\0';
        }
        return *this;
    }
};
} // namespace aprs
#endif
