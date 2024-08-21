//
// Created by Angel Dust on 17/04/2021.
//

#include "s_meter_widget.h"
#include "../config.h"
#include "s_strength.h"
#include "frequency_widget.h"
#include "view_manager.h"


void SMeterWidget::paint_callback() {


    char buf[6];
    int padding = 15;
    int block_size = (this->area.width - padding * 2) / MAX_S_LEVEL;
    int s9_x = (S_LEVELS * block_size);
    int max_x = s9_x + (DB_LEVELS * block_size);
    int x = (state.s_level / (float) MAX_S_LEVEL) * max_x;
    int peak_x = (state.peak_s_level / (float) MAX_S_LEVEL) * max_x;
    int y1 = S_METER_LINE_HEIGHT + 4;
    int y2 = y1 + S_METER_LINE_HEIGHT;
    uint16_t color = C565_WHITE;

    display->clear();

    display->setFont((FontDef *) &Font_Fixed5x7);

    display->setBgColor(C565_BLACK);

    // Tick values
    for (int level = 0; level <= MAX_S_LEVEL; level++) {
        buf[0] = 0;
        int px = block_size * level;
        int tick_size = 1;

        if (level == 0) {
            sprintf(buf, "S");
            color = C565_WHITE;
        } else if (level % 2 != 0) {
            tick_size = 2;
            if (level > S_LEVELS) {
                if (((level - 1) % 4 != 0)) {
                    sprintf(buf, "+%i", (level - S_LEVELS) * 10);
                    color = C565_RED;
                }
            } else {
                color = C565_GREY_LIGHT;
                sprintf(buf, "%i", level);
            }
        }

        if (buf[0]) {
            display->setColor(color);
            display->gotoXY(px + padding - ((int) strlen(buf) * 2), 1);
            display->write(buf);
        }

        // Tick
        display->writeLine(px + padding, y1 - 4, px + padding, y1 - 4 + tick_size, C565_GREY_DARK);
    }

    // Horizontal line
    display->writeLine(padding, y1 - 4, max_x + padding, y1 - 4, C565_GREY_DARK);

    // Bar
    for (int ix = 0; ix < x; ix++) {
        if (ix % block_size != 0) {
            if (ix > s9_x) {
                color = C565_RED;
            } else {
                color = C565_WHITE;
            }
            display->writeVertLine(ix + padding, y1, y2, color);
        }
    }

    // Peak
    display->writeVertLine(peak_x + padding, y1, y2, C565_CYAN);
    display->writeVertLine(peak_x + 1 + padding, y1, y2, C565_CYAN);

    // AGC flag
    if (config.agc_enabled) {
        display->setColor(C565_GREY_LIGHT);
        display->gotoXY(max_x - 15, y1 + 2);
        display->print("AGC");
    }
}

float SMeterWidget::get_s_level(float current, float smooth_factor) {
    float level = current - smooth_factor * (current - sstrength::s_level);
    level = fmax(level, 0);
    level = fmin(level, MAX_S_LEVEL);
    return level;
}

st_meter_widget_state SMeterWidget::get_state() {
    st_meter_widget_state new_state;
    new_state.s_level = get_s_level(state.s_level, 0.3);
    new_state.peak_s_level = fmax(get_s_level(state.peak_s_level, 0.03), new_state.s_level);
    return new_state;
}

void SMeterWidget::do_paint() {

    st_meter_widget_state current_state = get_state();

    if (this->dirty() || !(current_state == state)) { // Update only if status has changed
        this->set_dirty();
        state = current_state;
        display->drawArea(&this->area, this);
    }
}

bool SMeterWidget::on_input(const st_inputEvent event) {

    switch (event.type) {

        case INPUT_EVENT_TYPE_TOUCH_END:
            view_manager::keypadView.set_value(sstrength::get_squelch(), 2, "x1", "Squelch");
            view_manager::keypadView.with_multipliers(false);
            view_manager::keypadView.on_changed = [](double v) {
                sstrength::set_squelch((float) v);
            };
            view_manager::push(&view_manager::keypadView);
            return true;
    }

    return false;

}

