//
// Created by Angel Dust on 129/03/2025.
//

#include "snr_widget.h"
#include "config.h"
#include "dsp/fft/fft.h"
#include "status.h"

void SNRWidget::before_paint() {

    float curr_snr = fft::snr;
    float curr_dbm = fft::dbm;
    char buff[10];

    // Round to 1 decimal place
    curr_snr = roundf(curr_snr * 10.0f) / 10.0f;
    curr_dbm = roundf(curr_dbm * 10.0f) / 10.0f;

    if (curr_snr != snr) {
        snr = curr_snr;
        snprintf(buff, 6, "%5.1f", snr);
        lblSNR.set_value(buff);
    }

    if (curr_dbm != dbm) {
        dbm = curr_dbm;
        char buff[10];
        snprintf(buff, 7, "%6.1f", dbm);
        lblDbm.set_value(buff);
    }
}
