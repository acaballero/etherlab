//
// Created by Angel Dust on 16/04/2021.
//

#include "dsp_common.h"
#include "config.h"

const char *dsp_error_names[] = {"NONE", "ERROR", "FILEOPEN", "FILECLOSE", "FILEWRITE", "FILEREAD", "DMAOVERRUN", "FIFOOVERRUN", "FIFOUNDERRUN"};

st_dspStatus *dsp_status;

// Phase in the LUT table
// float dsp_lut_phase;

// Current maximum sample frequency. It depends on whether we're doing more or less real time processing to the ADC buffer
uint32_t dsp_max_sample_rate = config.fft.max_sample_rate;

Signal dsp_common_params_signal;

// TX gain for the digital domain
int8_t dsp_tx_gain = 0;

void set_max_sample_freq(bool dsp) {
    // Set the max sample frequency according to the amount of processing we will be doing
    if (!dsp) {
        dsp_max_sample_rate = config.fft.max_sample_rate;
    } else {
        dsp_max_sample_rate = config.fft.dsp_max_sample_rate;
    }
    fft_config(config.fft.span);
}

void set_max_sample_freq(uint32_t rate) {
    dsp_max_sample_rate = min2(config.fft.dsp_max_sample_rate, rate);
    fft_config(config.fft.span);
}

void set_tx_gain_db(int8_t gain_db) {
    dsp_tx_gain = constrain(gain_db, DSP_MIN_TX_GAIN_DB, DSP_MAX_TX_GAIN_DB);
    dsp_status->gain = pow(10.0, (float) dsp_tx_gain / 20.0);
    dsp_common_params_signal.emit(&dsp_status);
}