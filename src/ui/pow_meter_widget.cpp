//
// Created by Angel Dust on 17/04/2021.
//

#include <cfloat>
#include "pow_meter_widget.h"
#include "../config.h"

PowerMeterWidget::PowerMeterWidget(Rect parent_rect, Display *display) : Widget(parent_rect, display) {

    int max_watts = toWatts(max_dbm);
}

bool PowerMeterWidget::paint_callback() {

    display->setFont((FontDef *)&Font_Fixed5x7);
    display->setBgColor(C565_BLACK);
    display->gotoXY(0, 0);
    display->clear();

    char buf[20];
    sprintf(buf, "F:%.1f R:%.1f S:%.1f", info.p_for_dbm, info.p_ref_dbm, info.swr);

    display->print(buf);

    return true;
}

float PowerMeterWidget::toWatts(float dbm) {
    return dbm == -FLT_MAX ? 0 : pow(10, ((dbm - 30.0) / 10.0));
}

void PowerMeterWidget::before_paint() {

    rf_coupler::rf_coupler_info current_info{.v_for = 0, .v_ref = 0, .p_for_dbm = rf_coupler::info.p_for_dbm, .p_ref_dbm = 0, .swr = rf_coupler::info.swr};

    // Test
    // current_info.swr = max_swr;
    // current_info.p_for_dbm = max_dbm / 2;

    // Round to 2 decimals and constrain
    current_info.p_for_dbm = constrain(current_info.p_for_dbm, 0, max_dbm);
    current_info.p_for_dbm = (float)((int)(current_info.p_for_dbm * 100)) / (float)100;
    current_info.swr = constrain(current_info.swr, 1, max_swr);
    current_info.swr = (float)((int)(current_info.swr * 100)) / (float)100;

    if (this->dirty() || !(info == current_info)) { // Update only if status has changed
        this->set_dirty();
        info = current_info;
    }
}
