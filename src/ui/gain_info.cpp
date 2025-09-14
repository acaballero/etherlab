//
// Created by Angel Dust on 09/09/2025.
//

#include <cfloat>
#include <ios>
#include "agc.h"
#include "hw/board/board_v2.h"
#include "input/inputEvent.h"
#include "gain_info.h"
#include "../config.h"
#include "main_board.h"
#include "printf.h"
#include "rf_coupler.h"
#include "s_strength.h"
#include "stm32f4xx_hal.h"
#include "view.h"

bool GainInfoWidget::paint_callback() {
    char buf[50];

    display->clear();

    display->setBgColor(C565_BLACK);

    display->setFont((FontDef *)&Font_7x10);

    display->gotoCharXY(0, 0);

    sprintf(buf, "G:%d/%d/%.1f/%d/%d", main_board::get_frontend_gain(), agc::get_analog_gain(), agc::agc_voltage, board::if_gain_to_db(vga_gain),
            board::if_gain_to_db(vgb_gain));

    display->setBgColor(get_bg());

    display->print(buf);

    return true;
}

void GainInfoWidget::before_paint() {
    if (HAL_GetTick() % 100 < 3) {
        set_dirty();
    }
}

bool GainInfoWidget::on_touch(const st_inputEvent e) {
    return false;
}
