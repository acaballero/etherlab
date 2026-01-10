//
// Created by Angel Dust on 30/11/2025.
//
#include "beacon_settings_view.h"
#include "Display_afb.h"
#include "dsp/aprs/aprs_settings.h"
#include "dsp/dsp_common.h"
#include "hw/stm32f4xx/rtc.h"
#include "input/inputEvent.h"
#include "ips_font.h"
#include "status.h"
#include "io/config_file.h"
#include <cstddef>
#include <cstring>
#include <iterator>

namespace dsp_ui {

void BeaconSettingsView::init() {

    ConfigFile<aprs::settings> config_file;
    bool ok = config_file.load("aprs.cfg", &aprs_settings, true);

    gainField.set_value(dsp::dsp_config.gain);

    if (!ok) {
        status::pop_alert(status::ERROR, "Error reading/creating aprs.cfg");
        periodField.set_value(10);

    } else {
        periodField.set_value(aprs_settings.beacon_period_ms / 1000);
        deviationField.set_value(aprs_settings.deviation);
        messageField.set_text(aprs_settings.message);
    }

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

        aprs_settings.beacon_period_ms = periodField.get_value() * 1000;
        aprs_settings.deviation = deviationField.get_value();
        strncpy(aprs_settings.message, messageField.get_text().c_str(), aprs::max_message_length);
        aprs_settings.message[aprs::max_message_length - 1] = '\0';

        ConfigFile<aprs::settings> config_file;
        bool ok = config_file.save("aprs.cfg", &aprs_settings);

        if (!ok) {
            status::pop_alert(status::ERROR, "Error saving aprs.cfg");
        }

        if (on_select) {
            on_select(true, aprs_settings);
        }

        set_visible(false);
    };
    button_cancel.action = [this](Button &, st_inputEvent) {
        if (on_select) {
            aprs::settings settings{};
            on_select(false, settings);
        }
        set_visible(false);
    };

    // DEBUG focus
    periodField.set_name("perf");
    periodLabel.set_name("perl");
    gainField.set_name("ganf");
    gainLabel.set_name("ganl");
    button_cancel.set_name("btnc");
    button_ok.set_name("btno");

    Widget *widgets[] = {&periodLabel, &periodField, &gainLabel, &gainField, &deviationLabel, &deviationField, &messageLabel, &messageField};
    Label *labels[] = {&periodLabel, &gainLabel, &deviationLabel, &messageLabel};
    NumberField *fields[] = {&periodField, &gainField, &deviationField};

    for (auto w : widgets) {
        w->set_font((FontDef *)&Font_7x10);
        add_child(w);
    }

    for (auto w : labels) {
        w->resize();
    }

    for (auto w : fields) {
        w->set_fg(C565_FIELD_FG);
    }

    messageField.set_fg(C565_FIELD_FG);
    messageField.set_editable(true);

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

    // size_t i = 0;
    switch (e.type) {

        case INPUT_EVENT_TYPE_BUTTON_PRESS:
            switch (e.value) {

                case KEY_BACK:
                    button_cancel.action(button_cancel, e);
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
