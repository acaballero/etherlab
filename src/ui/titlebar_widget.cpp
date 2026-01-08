//
// Created by Angel Dust on 17/04/2021.
//

#include <scanner.h>
#include <stdint.h>
#include "Display_afb.h"
#include "dsp/dsp.h"
#include "dsp/dsp_common.h"
#include "stm32f4xx_hal.h"
#include "titlebar_widget.h"
#include "config.h"
#include "main_board.h"
#include "types.h"
#include "utils.hpp"

void TitleBarWidget::init() {

    btnDSP.action = [this](Button &, st_inputEvent) {
        main_board::toggle_dsp();
        set_dirty();
    };

    add_children({&btnDSP});
    set_name("tit_");

    for (Widget *btn : View::children()) {
        btn->set_font((FontDef *)&Font_7x10);
        btn->set_aling(ALIGN_CENTER);
        ((Button *)btn)->set_style(ButtonStyle::BUTTON_STYLE_FLAT);
        ((Button *)btn)->set_bg(C565_VIOLET);
        ((Button *)btn)->set_fg(C565_WHITE);
    }

    add_children({&titleBarWidgetInner});

    // Update every mode update event
    main_board::mode_signal.add(this, [this](void *, const void *) {
        set_dirty();
    });

    // And every clock tick, so the DSP status label is regularly updated
    rtc_signal.add(this, [this](void *, const void *) {
        set_dirty();
    });
}

void TitleBarWidget::before_paint() {

    if (dirty()) {

        uint16_t color = C565_BLACK;

        color = C565_GREY_LIGHT;

        display->set_padding(8, 8);
        btnDSP.set_bg(ISTX ? C565_RED : C565_VIOLET);

        if (ISANALOG) {
            btnDSP.set_text("ANA");
        } else {
            float drop_freq = dsp::dsp_params && dsp::dsp_params->status == DSP_STATUS_RUNNING ? dsp::dsp_params->drop_rate() : 0;
            float starve_freq = dsp::dsp_params && dsp::dsp_params->status == DSP_STATUS_RUNNING ? dsp::dsp_params->starve_rate() : 0;
            bool error = true;

            if (!dsp::dsp_params) {
                color = C565_GREY_DARKER;
                error = false;
            } else if (dsp::dsp_params->error != DSP_ERR_NONE || drop_freq * 100 > 1 || starve_freq * 100 > 1) {
                color = C565_ORANGE;
            } else if (drop_freq * 100 > 0.1 || starve_freq * 100 > 0.1) {
                color = C565_YELLOW;
            } else {
                error = false;
            }

            dsp::dsp_params->reset();
            char buf[20];
            MODULATION_MODE mod = main_board::get_modulation_mode();
            bool space = dsp::apply_compression(mod) || dsp::apply_deemph(mod) || dsp::apply_audio_bpf() || dsp::dsp_config.baseband_echo;
            sprintf(buf, "%s%s%s%s%s%s%s", "DSP", space ? " " : "", dsp::apply_compression(mod) ? "C" : "", dsp::apply_deemph(mod) ? "D" : "",
                    dsp::apply_audio_bpf() ? "F" : "", dsp::dsp_config.baseband_echo ? "E" : "", error ? " !" : "");
            trim(buf);
            btnDSP.set_text(buf);
        }

        btnDSP.set_fg(color);
    }
}
