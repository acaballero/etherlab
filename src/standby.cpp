//
// Created by Angel Dust on 21/06/2024.
//

#include "standby.h"
#include "config.h"
#include "dsp/fft/fft.h"
#include "os/periodic_task.h"
#include "os/task_manager.h"
#include "ui/lcd.h"
#include "hw/stm32.h"
#include "main_board.h"
#include "ui/main_view.h"
#include "status.h"
#include "ui/view_manager.h"
#include <cstddef>

//#include "hw/stm32.h"

namespace standby {

Signal signal;
POWER_MODE power_mode = POWER_MODE_ON;

os::periodic_task *power_save_timeout;

void init() {
    power_save(config.power_save_period_seconds);
}

void enable_display(bool b) {
    if (b) {
        lcd_init();
        fft::fft_task.set_enabled(true);
        view_manager::mainView.set_dirty();
        view_manager::task.set_enabled(true);
    } else {
        lcd_sleep();
        view_manager::task.set_enabled(false);
        fft::fft_task.set_enabled(false);
    }
}

int sleep() {

    int ret = 0;

    if (power_mode == POWER_MODE_ON && !ISTX) {
        main_board::sleep();
        ret = power_down_lo_clocks();
        enable_display(false);
        hal_sleep();
        power_mode = POWER_MODE_SLEEP;
        signal.emit(NULL);
    }

    if (ret < 0) {
        status::pop_alert(status::ST_ERROR, "Error powering down devide");
    }

    return ret;
}

int power_save(int timeout_seconds) {

    if (power_save_timeout) {
        if (os::task_manager.remove(power_save_timeout)) {
            delete power_save_timeout;
        }
    }

    if (timeout_seconds) {
        power_save_timeout = os::task_manager.set_timeout(timeout_seconds * 1000, []() {
            enable_display(false);
            power_mode = POWER_MODE_SAVE;
            signal.emit(NULL);
        });
    } else {
        power_mode = POWER_MODE_ON;
    }

    return 0;
}

int wakeup() {

    int ret = 0;

    if (power_mode != POWER_MODE_ON) {
        if (power_mode == POWER_MODE_SLEEP) {
            hal_wakeup();

            ret = power_up_lo_clocks();
            main_board::wakeup();
        } else if (power_mode == POWER_MODE_SAVE) {

            power_save(config.power_save_period_seconds);
        }

        enable_display(true);
        power_mode = POWER_MODE_ON;
        signal.emit(NULL);
    }

    return ret;
}
} // namespace standby
