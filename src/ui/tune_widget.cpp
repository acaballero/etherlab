//
// Created by Angel Dust on 17/04/2021.
//

#include <cfloat>
#include <ios>
#include "input/inputEvent.h"
#include "tune_widget.h"
#include "../config.h"
#include "rf_coupler.h"
#include "s_strength.h"
#include "view.h"

bool TuneWidget::paint_callback() {

    char buf[50];

    display->clear();

    /*if (si5351.dev_status.LOS) {

        display->gotoCharXY(1, 2);
        display->print("* CLK FAIL ");
        display->print(si5351.dev_status.LOS);
        display->print(" *");

    }
    else */

    //#if SI5351_ENABLED
    //    if (si5351.dev_status.LOL_A || si5351.dev_status.LOL_B) {
    //
    //        display->gotoCharXY(1, 2);
    //        display->print("*PLL LOCK FAIL*");
    //
    //    }

    //#endif

    /*
        display->gotoCharXY(1, 2);
        display->print(direction);
        */

    display->setBgColor(C565_BLACK);
    display->setFont((FontDef *)&Font_7x10);

    format_long(radio::mixers[0].getLo() / 1000, buf);
    display->gotoCharXY(0, 0);
    display->print("LO1: ", buf, " kHz");

    format_long(radio::mixers[1].getLo() / 1000, buf);
    display->print(" LO2: ", buf, " kHz");

    if (ISTX) {

        // setFont(FONT3X5);
        display->gotoCharXY(0, 1);

        sprintf(buf, "F:%.2f R:%.1f", rf_coupler::info.v_for, rf_coupler::info.v_ref);
        display->print(buf);

        if (rf_coupler::info.p_for_dbm == -FLT_MAX) {
            sprintf(buf, " dBm:-");
        } else {
            sprintf(buf, " W:%.1f dBm:%d", rf_coupler::toWatts(rf_coupler::info.p_for_dbm), (int)rf_coupler::info.p_for_dbm);
        }

        display->print(buf);

        if (rf_coupler::info.swr == FLT_MAX) {
            // 0 return loss
            sprintf(buf, " SWR:MAX");
        } else if (rf_coupler::info.swr == 0) {
            // Undefined
            sprintf(buf, " SWR:?");
        } else {
            sprintf(buf, " SWR:%.1f", rf_coupler::info.swr);
        }

        display->print(buf);
        //   setFont(FONT5X8);
    } else {

        // Low pass
        s_level = s_level - 0.1 * (s_level - sstrength::s_level);
        s_level = fmax(s_level, 0);

        sprintf(buf, "%d.%d", (int)(s_level), (int)(s_level * 10) % 10);

        display->setFont((FontDef *)&Font_7x10);
        display->gotoCharXY(0, 1);
        display->print("S: ", buf, "");

        for (int is = 0; is < s_level; is++) {
            if (is > 9) {
                display->setColor(C565_RED);
            } else {
                display->setColor(C565_WHITE);
            }
            display->print("~");
        }

        // Show voltage

        // display->gotoCharXY(20, 1);
        // display->print(" ");
        // display->print(sstrength::s_strength);
    }

    return true;
}

void TuneWidget::before_paint() {
    this->set_dirty();
}

bool TuneWidget::on_touch(const st_inputEvent e) {
    if (e.ms > LONG_PRESS_MS) {
        config.debug = false;
        return true;
    }

    return false;
}
