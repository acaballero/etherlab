//
// Created by Angel Dust on 17/04/2021.
//

#include "s_meter_widget.h"
#include "../config.h"
#include "input/inputEvent.h"
#include "ips_font.h"
#include "s_strength.h"
#include "frequency_widget.h"
#include "stm32f4xx_hal.h"
#include "view_manager.h"
#include "menu_prompts.h"

void SMeterWidget::paint_callback() {

    char buf[6];
    int padding = 15;
    int block_size = (float)(this->area.box.width - padding * 2) / (float)MAX_S_LEVEL;
    int s9_x = (S_LEVELS * block_size);
    int max_x = s9_x + (DB_LEVELS * block_size) + padding;
    int x = (state.s_level / (float)MAX_S_LEVEL) * max_x;
    int peak_x = (state.peak_s_level / (float)MAX_S_LEVEL) * max_x;
    FontDef *font = (FontDef *)&Font_Fixed5x7;
    int font_h = font->height;
    int y1 = margin_top + font_h + 1;
    int major_tick_size = 2;
    int y2 = y1 + S_METER_LINE_HEIGHT + 2 * (major_tick_size + 1);

    uint16_t color = C565_WHITE;

    display->clear();

    display->setFont(font);

    display->setBgColor(C565_BLACK);

    // Tick values
    for (int level = 0; level <= MAX_S_LEVEL; level++) {
        buf[0] = 0;
        int px = block_size * level + padding;
        int tick_size = 1;

        if (level == 0) {
            sprintf(buf, "S");
            color = C565_WHITE;
        } else if (level % 2 != 0) {
            tick_size = major_tick_size;
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
            display->gotoXY(px - (((int)strlen(buf) * font->width) / 2), margin_top);
            display->write(buf);
        }

        // Tick
        display->writeLine(px, y1, px, y1 + tick_size, C565_GREY_DARK);
        display->writeLine(px, y2 - tick_size, px, y2, C565_GREY_DARK);
    }

    // Horizontal lines
    display->writeLine(padding, y1, max_x, y1, C565_GREY_DARK);
    display->writeLine(padding, y2, max_x, y2, C565_GREY_DARK);

    // Bar
    for (int ix = 0; ix < x; ix++) {
        if (ix % block_size != 0) {
            if (ix > s9_x) {
                color = C565_RED;
            } else {
                color = C565_WHITE;
            }
            display->writeVertLine(ix + padding, y1 + major_tick_size + 1, y2 - major_tick_size - 1, color);
        }
    }

    // Peak
    display->writeVertLine(peak_x + padding, y1 + major_tick_size + 1, y2 - major_tick_size - 1, C565_CYAN);
    display->writeVertLine(peak_x + 1 + padding, y1 + major_tick_size + 1, y2 - major_tick_size - 1, C565_CYAN);

    // AGC flag
    if (config.agc_enabled) {
        display->setColor(C565_GREY_LIGHT);
        display->setFont((FontDef *)&Font_Fixed5x7);
        display->gotoXY(max_x - (display->getFont()->width * 3) - 5, y1 + (((y2 - y1) - display->getFont()->height + 1) / 2));
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
    new_state.peak_s_level = fmax(get_s_level(state.peak_s_level, 0.08), new_state.s_level);
    return new_state;
}

void SMeterWidget::before_paint() {

    st_meter_widget_state current_state = get_state();

    if (!(current_state == state)) {
        state = current_state;
    }

    // Limit update rate
    uint64_t t = HAL_GetTick();
    if (t - last_update_ms > update_period_ms) {
        last_update_ms = t;
        this->set_dirty();
    }
}

bool SMeterWidget::on_input(const st_inputEvent event) {

    switch (event.type) {

        case INPUT_EVENT_TYPE_TOUCH_END:

            if (event.ms > LONG_PRESS_MS) { // Long press
                config.debug = true;
            } else {
                Menu::open_keypad<float>(
                    sstrength::get_squelch(), "x1", "Squelch", 1, false,
                    [](float v) {
                        sstrength::set_squelch((float)v);
                    },
                    0, 9);
            }
            return true;
        default:
            return false;
    }
}
