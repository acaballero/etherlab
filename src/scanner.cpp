//
// Created by Angel Dust on 23/06/2021.
//

#include "scanner.h"
#include "config.h"
#include <ctime>
#include <hw/stm32.h>
#include "os/periodic_task.h"
#include "radio.h"
#include "s_strength.h"
#include "printf.h"
#include "types.h"
#include "ui/frequency_memory_ui.h"

namespace scanner {

enum STATE { SEARCHING, PEAKING, EXITING };

void sweep();

st_scanner_info scanner_config;

Signal signal;

// Time (ms) the scan has been paused
unsigned long sweep_pause_time_ms = 0;

// Last detected signal's vfo config
st_vfo_config last_vfo_config;

// Current detected signal strength
float sstrength = 0;

int nsaved = 0;

radio::BAND current_band;

// Current state. Default is EXITING to start discarding a currently tuned signal, if any.
STATE state = EXITING;

os::periodic_task task(50, sweep);

void start() {

    current_band = radio::get_curr_freq_band();
    if (scanner_config.mode != SCANNER_MODE_BAND) {
        // Don't constraint to current band

        radio::set_band(radio::BAND_ALL);
    }
}

void stop() {

    if (scanner_config.status != SCANNER_STATUS_STOPPED) {
        // Restore the band
        if (radio::get_band() != current_band) {
            radio::set_band(current_band);
        }

        scanner_config.status = SCANNER_STATUS_STOPPED;
        state = EXITING;
        signal.emit(&scanner_config);
    }

    nsaved = 0;
}

void toggle() {

    if (scanner_config.freq_min == 0) {
        nav.doNav(Menu::navCmd(Menu::enterCmd));
        nav.doNav(Menu::navCmd(Menu::idxCmd, 2));
    } else {
        if (scanner_config.status == SCANNER_STATUS_RUNNING && scanner_config.direction == FORWARD) {
            scanner_config.direction = BACKWARDS;
        } else if (scanner_config.status != SCANNER_STATUS_STOPPED) {
            stop();
        } else {

            scanner_config.status = SCANNER_STATUS_RUNNING;
            scanner_config.direction = FORWARD;
            start();
        }
    }
}

void configure(st_scanner_info &config) {

    if (config != scanner_config) {

        bool valid = config.period_s && config.freq_max > config.freq_min + config.freq_step;

        if (!valid) {
            config.freq_max = config.freq_min + config.freq_step;
            config.period_s = 1;
        }

        // Adjust to multiples of the step
        config.freq_min = (config.freq_min / config.freq_step) * config.freq_step - config.freq_step / 2;
        config.freq_max = (config.freq_max / config.freq_step) * config.freq_step + config.freq_step / 2;

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

void generate_string(char *buffer, size_t size, int number) {
#if ENABLE_RTC
    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;
    HAL_RTC_GetTime(&hrtc, &time, FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &date, FORMAT_BIN);

    // Format the timestamp in RFC 3339 format
    snprintf(buffer, size, "%02d%02d%02d", date.Year, date.Month, date.Date);
#else
    snprintf(buffer, size, "scanned", date.Year, date.Month, date.Date);
#endif

    // Append the integer to the formatted timestamp
    snprintf(buffer + strlen(buffer), size - strlen(buffer), "_%d", number);
}

void sweep() {

    uint64_t t = HAL_GetTick();

    if (scanner_config.freq_min && scanner_config.status != SCANNER_STATUS_STOPPED &&
        (sweep_pause_time_ms == 0 || (t - sweep_pause_time_ms) > scanner_config.pause_ms)) {

        sweep_pause_time_ms = 0;

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
                        printf_("Pausing after PEAK: s: %.2f, last: %.2f", s, sstrength);
                        sweep_pause_time_ms = t;

                        last_vfo_config = config.vfo[radio::get_vfo()];

                        state = EXITING;

                        uint64_t freq = radio::get_frequency();
                        if (scanner_config.save_found) {
                            char name[FREQ_MEM_NAME_SIZE];
                            generate_string(name, sizeof(name), nsaved++);
                            st_freq_mem f{0, freq, config.modulation, name};
                            freq_memory::save(f);
                        }

                    } else {
                        printf_("Stopping after PEAK: s: %.2f, last: %.2f", s, sstrength);
                        stop();
                    }
                }

                sstrength = s;
                break;

            case EXITING:

                // If we are inside a signal bandwidth, we keep exiting until the strength is below the
                // squelch. However, if the squelch is below the noise, we would stay in this state forever, so
                // we set a limit

                if (std::abs((int64_t)(last_vfo_config.freq - config.vfo[radio::get_vfo()].freq)) > scanner_config.freq_step * 2) {
                    printf_("EXITING: s: %.2f, last: %.2f", s, sstrength);
                    state = SEARCHING;
                    last_vfo_config = {0, 0};
                }

                break;

            default:

                if (s >= scanner_config.squelch) {
                    printf_("PEAKING: s: %.2f, last: %.2f", s, sstrength);
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
