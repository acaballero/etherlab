//
// Created by Angel Dust on 17/04/2021.
//

#include "info_widget.h"
#include "config.h"
#include "s_strength.h"
#include "status.h"
#include "agc.h"

bool InfoWidget::paint_callback() {

    uint8_t buf_size = 30;
    char buf[buf_size];

    display->clear();

    display->setBgColor(C565_BLACK);
    display->setColor(C565_WHITE);
    display->setFont((FontDef *)&Font_7x10);
    display->set_padding(0, 0);
    display->gotoCharXY(0, 0);
    format_long(fft::fft_params.span / 1000, buf);

    display->print("Span:", buf, " kHz");

    uint16_t fft_fs = fft::fft_params.sample_freq / 1000 / fft::fft_params.decimation_factor;

    display->gotoCharXY(13, 0);

    format_long(fft_fs, buf);
    display->print("ADC:", buf, " kHz");

    if (fft::fft_params.decimation_factor > 1) {

        snprintf(buf, 6, "[x%d]", fft::fft_params.decimation_factor);
        display->print(buf);
    }

    display->gotoCharXY(0, 1);

    snprintf(buf, 12, " #: %d", fft::fft_params.n_slices);
    display->print(buf);

    snprintf(buf, 6, " %.0f", fft::fft_params.display_rbw);
    display->print(" RBW:", buf, " Hz");

    display->gotoCharXY(0, 2);

    format_long(fft_peak_f, buf);

    display->print("Peak:", buf, " Hz");

    snprintf(buf, 12, " %.1f dB", fft_peak);
    display->print(buf);

    snprintf(buf, 6, "%4d", (int)fft::fft_noise_floor_db);
    display->print(" N.Floor: ", buf, " dB");

    display->gotoCharXY(0, 3);

    format_long(radio::f_dsp_if / 1000, buf);

    display->print("IF:", buf, " kHz");

    snprintf(buf, 4, "%.1f", sstrength::db_to_s_strength(fft::fft_noise_floor_db));
    display->print(" N.Floor S: ", buf, "");

    // snprintf(buf, 4, "%d", agc::get_gain());
    // display->print(" Gain: ", buf, "dB");

    snprintf(buf, 4, "%.1f", agc::agc_voltage);
    display->print(" AGC: ", buf, " V");

    if (status::systemStatus.code != status::ST_OK) {
        display->gotoCharXY(0, 4);
        display->setColor(C565_RED);
        display->print(status::systemStatus.msg);
    }

    display->set_padding(4, 4);
    return true;
}

void InfoWidget::before_paint() {
}
