//
// Created by Angel Dust on 23/06/2021.
//

#ifndef TRX_FRONTEND_SCANNER_H
#define TRX_FRONTEND_SCANNER_H

#include "stdio.h"
#include "types.h"

#define SCANNER_MIN_DELAY_MS 40
#define SCANNER_MAX_DELAY_MS 1000

namespace scanner {

    enum SCANNER_STATUS {
        SCANNER_STATUS_STOPPED, SCANNER_STATUS_RUNNING
    };

    enum SCANNER_MODE {
        SCANNER_MODE_CUSTOM, SCANNER_MODE_BAND, SCANNER_MODE_LIST
    };

    struct st_scanner_info {

        uint64_t freq_min{0};
        uint64_t freq_max{0};
        uint32_t freq_step{1000};
        float squelch{3};
        uint32_t pause_ms{1000};
        uint16_t period_s{1000};
        DIRECTION direction{STOP};
        SCANNER_MODE mode{SCANNER_MODE_CUSTOM};
        SCANNER_STATUS status{SCANNER_STATUS_STOPPED};

        bool operator==(const st_scanner_info &st) const {
            return freq_min == st.freq_min
                   && freq_max == st.freq_max
                   && freq_step == st.freq_step
                   && status == st.status
                   && squelch == st.squelch
                   && pause_ms == st.pause_ms
                   && period_s == st.period_s
                   && direction == st.direction;
        }

        bool operator!=(const st_scanner_info &st) const {
            return !(*this == st);
        }
    };

    extern st_scanner_info scanner_config;
    extern Signal signal;

    void stop();

    void loop();

    void configure(st_scanner_info&);
}

#endif //TRX_FRONTEND_SCANNER_H
