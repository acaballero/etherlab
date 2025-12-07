//
// Created by Angel Dust on 30/11/2025.
//
#include "beacon_settings_view.h"
#include "Display_afb.h"
#include "dsp/dsp_common.h"
#include "hw/stm32f4xx/rtc.h"
#include "input/inputEvent.h"
#include "ips_font.h"
#include "status.h"
#include <cstddef>
#include <cstring>
#include <iterator>

namespace dsp_ui {

void BeaconSettingsView::init() {

    periodField.set_value(5);

    gainField.set_value(dsp::dsp_config.gain);

    int bw = get_border_width();
    int sw = get_shadow_width();
    int padding = bw + sw;
    int w = width - padding * 2;

    int button_width = w / 2 - 1;
    int button_height = 36;

    Rect r = parent_rect();
    int button_top = r.height() - padding - button_height;

    button_ok.set_parent_rect({padding, button_top, button_width, button_height});
    button_cancel.set_parent_rect({padding + button_width, button_top, button_width, button_height});

    button_ok.action = [this](Button &, st_inputEvent) {
        dsp::dsp_config.gain = gainField.get_value();

        if (on_select) {
            on_select(true);
        }

        set_visible(false);
    };
    button_cancel.action = [this](Button &, st_inputEvent) {
        if (on_select) {
            on_select(false);
        }
        set_visible(false);
    };

    Widget *texts[] = {&periodLabel, &periodField, &gainLabel, &gainField};
    Label *labels[] = {&periodLabel, &gainLabel};
    NumberField *fields[] = {&periodField, &gainField};

    for (auto w : texts) {
        w->set_font((FontDef *)&Font_11x18);
        add_child(w);
    }

    for (auto w : labels) {
        w->resize();
    }

    for (auto w : fields) {
        w->set_fg(C565_FIELD_FG);
    }

    add_children({&button_ok, &button_cancel});

    button_ok.set_focus(true);
}

bool BeaconSettingsView::on_touch(const st_inputEvent e) {

    return true;
}

bool BeaconSettingsView::on_input(const st_inputEvent e) {

    bool consumed = Widget::on_input(e);

    if (consumed) {
        return true;
    }

    size_t i = 0;
    switch (e.type) {

        case INPUT_EVENT_TYPE_ENCODER:

            consumed = true;
            break;

        case INPUT_EVENT_TYPE_BUTTON_PRESS:
            switch (e.value) {
                case BTN_ENCODER:

                    consumed = true;

                    break;
                default:
                    consumed = false;
                    break;
            }
            break;

        default:
            consumed = false;
            break;
    }

    return consumed;
}

} // namespace dsp_ui
