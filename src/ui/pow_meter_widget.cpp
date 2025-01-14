//
// Created by Angel Dust on 17/04/2021.
//

#include <cfloat>
#include "pow_meter_widget.h"
#include "../config.h"

PowerMeterWidget::PowerMeterWidget(Rect parent_rect, Display *display) : Widget(parent_rect, display) {

    float f_swr_block_size = 0.1;
    float f_dbm_block_size = 0.1;

    dbm_nblocks = ((float)max_dbm / (float)dbm_tick_spacing);

    while (f_swr_block_size != round(f_swr_block_size) || f_dbm_block_size != round(f_dbm_block_size)) {
        // Adjust the margin so the block size is an integer number of pixels for both bars
        f_swr_block_size = ((float)(this->area.width - margin_right - margin) / ((float)max_swr - 1.0));
        f_dbm_block_size = ((float)(this->area.width - margin_right - margin) / (float)dbm_nblocks);
        margin_right++;
    }

    dbm_block_size = f_dbm_block_size;
    swr_block_size = f_swr_block_size;
}

void PowerMeterWidget::paint_callback() {
    display->clear();
    display->setFont((FontDef *)&Font_Fixed5x7);
    display->setBgColor(C565_BLACK);

    paint_power();
    paint_swr();
}

void PowerMeterWidget::paint_power() {
    char buf[6];

    int max_x = margin + (dbm_nblocks * dbm_block_size);
    int x = (info.p_for_dbm / (float)max_dbm) * max_x;
    int y1 = margin_top + 4;
    int y2 = y1 + DBM_BAR_HEIGHT;
    uint16_t color;

    // Tick values
    for (int level = 0; level <= dbm_nblocks; level++) {
        buf[0] = 0;
        int px = dbm_block_size * level + margin;
        int tick_size = 1;

        if (level == 0) {
            sprintf(buf, "dBm");
            color = C565_WHITE;
        } else if (level % 2 == 0) {
            tick_size = 2;
            color = C565_GREY_LIGHT;
            sprintf(buf, "%i", level * dbm_tick_spacing);
        }

        if (buf[0]) {
            display->setColor(color);
            display->gotoXY(px, 1);
            display->write(buf);
        }

        // Tick
        display->writeLine(px, y1 - 4, px, y1 - 4 + tick_size, C565_GREY_DARK);
    }

    // Horizontal line
    display->writeLine(margin, y1 - 4, max_x, y1 - 4, C565_GREY_DARK);

    // Bar
    for (int ix = margin; ix < x; ix++) {
        if ((ix - margin) % (dbm_block_size) != 0) {
            display->writeVertLine(ix, y1, y2, C565_WHITE);
        }
    }
}

void PowerMeterWidget::paint_swr() {
    char buf[6];

    int max_x = margin + (max_swr - 1) * swr_block_size;
    int x = ((info.swr - 1.0) / ((float)max_swr - 1.0)) * (float)max_x;
    int y1 = margin_top + 4 + DBM_BAR_HEIGHT + 2;
    int y2 = y1 + SWR_BAR_HEIGHT;
    uint16_t color;

    // Tick values
    for (int level = 1; level <= max_swr; level++) {
        buf[0] = 0;
        int px = swr_block_size * (level - 1);
        int tick_size = 1;

        if (level == 1) {
            sprintf(buf, "SWR");
            color = C565_WHITE;
        } else {
            tick_size = 2;
            color = C565_GREY_LIGHT;
            sprintf(buf, "%i", level);
        }

        if (buf[0]) {
            display->setColor(color);
            display->gotoXY(max_x - px - (int)strlen(buf) * 3, y2 + 6);
            display->write(buf);
        }

        // Tick
        display->writeLine(max_x - px, y2 + 3 - tick_size, max_x - px, y2 + 3, C565_GREY_DARK);
    }

    // Horizontal line
    display->writeLine(margin, y2 + 4, max_x, y2 + 4, C565_GREY_DARK);

    // Bar
    for (int ix = 0; ix < x; ix++) {
        if (ix % (swr_block_size) != 0) {
            int iblock = ix / swr_block_size;
            if (iblock > 1) {
                color = C565_RED;
            } else if (iblock > 0) {
                color = C565_YELLOW;
            } else {
                color = C565_WHITE;
            }
            display->writeVertLine(max_x - ix, y1, y2, color);
        }
    }
}

void PowerMeterWidget::before_paint() {

    rf_coupler::rf_coupler_info current_info{.v_for = 0, .v_ref = 0, .p_for_dbm = rf_coupler::info.p_for_dbm, .p_ref_dbm = 0, .swr = rf_coupler::info.swr};

    // Round to 2 decimals and constrain
    current_info.p_for_dbm = constrain(current_info.p_for_dbm, 0, max_dbm);
    current_info.p_for_dbm = (float)((int)(current_info.p_for_dbm * 100)) / (float)100;
    current_info.swr = constrain(current_info.swr, 1, max_swr);
    current_info.swr = (float)((int)(current_info.swr * 100)) / (float)100;

    current_info.swr = 2.5;
    current_info.p_for_dbm = max_dbm / 2;

    if (true || this->dirty() || !(info == current_info)) { // Update only if status has changed
        this->set_dirty();
        info = current_info;
    }
}
