//
// Created by Angel Dust on 23/06/2021.
//

#include "scanner.h"
#include "config.h"
#include <hw/stm32.h>
#include "os/periodic_task.h"
#include "radio.h"
#include "s_strength.h"

namespace scanner {

enum STATE { SEARCHING, PEAKING, EXITING };

void sweep();

st_scanner_info scanner_config;

Signal signal;

// Time (ms) the scan has been paused
unsigned long sweep_delay_time_ms = 0;

// Current detected signal strength
float sstrength = 0;

radio::BAND current_band;

// Current state. Default is EXITING to start discarding a currently tuned signal, if any.
STATE state = EXITING;

os::periodic_task task(50, sweep);

void start() {

    current_band = config.band;
    if (scanner_config.mode != SCANNER_MODE_BAND) {
        // Don't constraint to current band
        config.band = radio::BAND_ALL;
        radio::set_band();
    }
}

void stop() {

    if (scanner_config.status != SCANNER_STATUS_STOPPED) {
        // Restore the band
        if (config.band != current_band) {
            config.band = current_band;
            radio::set_band();
        }

        scanner_config.status = SCANNER_STATUS_STOPPED;
        state = EXITING;
        signal.emit(&scanner_config);
    }
}

void configure(st_scanner_info &config) {

    if (config != scanner_config) {

        bool valid = config.period_s && config.freq_max > config.freq_min + config.freq_step;

        if (!valid) {
            config.freq_max = config.freq_min + config.freq_step;
            config.period_s = 1;
        }

        uint32_t steps = ((float)(config.freq_max - config.freq_min) / (float)config.freq_step);

        uint32_t task_period_ms = (float)config.period_s * 1000 / (float)steps;

        if (task_period_ms < SCANNER_MIN_DELAY_MS) {
            task_period_ms = SCANNER_MIN_DELAY_MS;
            config.period_s = max2(1, steps * task_period_ms / 1000);
        }

        scanner_config = config;
        task.set_period(task_period_ms);

        if (config.status == SCANNER_STATUS_RUNNING) {
            start();
        }
    }
}

void step(DIRECTION direction) {
    unsigned long f = radio::get_frequency();

    f += (direction == FORWARD ? scanner_config.freq_step : -scanner_config.freq_step);

    if (f > scanner_config.freq_max) {
        f = scanner_config.freq_min;
    } else if (f < scanner_config.freq_min) {
        f = scanner_config.freq_max;
    }

    radio::set_frequency(f);
    radio::update_freq();
}

void sweep() {

    uint64_t t = HAL_GetTick();

    if (scanner_config.freq_min && scanner_config.status != SCANNER_STATUS_STOPPED &&
        (sweep_delay_time_ms == 0 || (t - sweep_delay_time_ms) > scanner_config.pause_ms)) {

        sweep_delay_time_ms = 0;

        // Here we get the strength without filtering to catch sudden rises in its value
        float s = sstrength::update_s_strength();
        DIRECTION direction = scanner_config.direction;

        switch (state) {

            case PEAKING:

                if (s < sstrength) {
                    // The signal strength starts decreasing, so we take a step backwards and pause or stop

                    // Step back
                    direction = scanner_config.direction == FORWARD ? BACKWARDS : FORWARD;

                    if (scanner_config.pause_ms) {
                        sweep_delay_time_ms = t;
                        state = EXITING;
                    } else {
                        stop();
                    }
                }
                break;

            case EXITING:

                // If we are inside a signal bandwidth, we keep exiting until the strength is below the
                // squelch. However, if the squelch is below the noise, we would stay in this state forever, so
                // we set a limit

                if (s < scanner_config.squelch || ((t < sweep_delay_time_ms) && (sweep_delay_time_ms > (scanner_config.pause_ms * 10)))) {
                    state = SEARCHING;
                }
                break;

            default:

                if (s >= scanner_config.squelch) {
                    sstrength = s;
                    state = PEAKING;
                }
                break;
        }

        sstrength = s;

        step(direction);
    }
}

} // namespace scanner
