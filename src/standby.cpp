//
// Created by Angel Dust on 21/06/2024.
//

#include "standby.h"
#include "ui/lcd.h"
#include "hw/stm32.h"
#include "main_board.h"
#include "ui/main_view.h"
#include "status.h"
#include "ui/view_manager.h"

//#include "hw/stm32.h"

namespace standby {

    Signal signal;
    POWER_MODE power_mode = POWER_MODE_ON;


    int sleep() {

        int ret=0;

        if (power_mode == POWER_MODE_ON && !ISTX) {
            main_board::sleep();
            ret = power_down_lo_clocks();
            lcd_sleep();
            hal_sleep();
            power_mode = POWER_MODE_SLEEP;
            signal.emit(NULL);
        }

        if (ret<0) {
            status::handleError(status::ST_ERROR,"Error powering down devide");
        }

        return ret;
    }

    int wakeup() {

        int ret=0;

        if (power_mode == POWER_MODE_SLEEP) {
            hal_wakeup();
            lcd_init();
            ret = power_up_lo_clocks();
            main_board::wakeup();
            view_manager::mainView.set_dirty();
            power_mode = POWER_MODE_ON;
            signal.emit(NULL);
        }

        return ret;
    }
}
