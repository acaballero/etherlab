//
// Created by Angel Dust on 17/04/2021.
//

#include <cfloat>
#include "ips_font.h"
#include "power_metrics_widget.h"
#include "../config.h"

PowerMetricsWidget::PowerMetricsWidget(Rect parent_rect, Display *display) : Widget(parent_rect, display) {
}

bool PowerMetricsWidget::paint_callback() {

    display->setFont((FontDef *)&Font_7x10);
    display->setBgColor(C565_BLACK);
    display->gotoXY(0, 8);
    display->clear();

    char buf[5];
    sprintf(buf, "%.1f", info.p_for_dbm);
    display->print("F", buf, "");
    sprintf(buf, "%.1f", info.p_ref_dbm);
    display->print(" R", buf, "");
    if (info.swr >= 1) {
        sprintf(buf, "%.1f", info.swr);
    } else {
        sprintf(buf, "-");
    }
    display->print(" S", buf, "");

    return true;
}

float PowerMetricsWidget::toWatts(float dbm) {
    return dbm == -FLT_MAX ? 0 : pow(10, ((dbm - 30.0) / 10.0));
}

void PowerMetricsWidget::before_paint() {

    rf_coupler::rf_coupler_info current_info{.v_for = 0, .v_ref = 0, .p_for_dbm = rf_coupler::info.p_for_dbm, .p_ref_dbm = 0, .swr = rf_coupler::info.swr};

    // Test
    // current_info.swr = max_swr;
    // current_info.p_for_dbm = max_dbm / 2;

    // Round to 2 decimals and constrain
    current_info.p_for_dbm = constrain(current_info.p_for_dbm, 0, 100);
    current_info.p_for_dbm = (float)((int)(current_info.p_for_dbm * 100)) / (float)100;

    current_info.swr = (float)((int)(current_info.swr * 100)) / (float)100;

    if (this->dirty() || !(info == current_info)) { // Update only if status has changed
        this->set_dirty();
        info = current_info;
    }
}
