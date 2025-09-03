//
// Created by Angel Dust on 19/04/2021.
//
#ifndef TRX_FRONTEND_FFT_TYPES_H
#define TRX_FRONTEND_FFT_TYPES_H

#include "../../../lib/utils/utils.hpp"
#include "Display_afb.h"
#include "hw/stm32.h"
#include "dsp/dsp_common.h"

enum FFT_SPECTRUM_STYLE { FFT_SPECTRUM_STYLE_FILL, FFT_SPECTRUM_STYLE_LINE, FFT_SPECTRUM_STYLE_LINE_FILL };

#define FFT_TYPE FFT_TYPE_FLOAT

#define FFT_IQBALANCE_REFRESH_PERIOD_MS 1000
#define FFT_SNR_REFRESH_PERIOD_MS 150
#define FFT_WATERFALL_NCOLORS 16
#define FFT_WATERFALL_MIN_REFRESH_PERIOD_MS 75
#define FFT_WATERFALL_MAX_PIXELS_PER_FRAME 4 // max scrolled pixels per frame

// Length (number of bins) of a single fourier transform
#define FFT_N 256

// Needs to be >= DSP_BANDWIDTH*2 by a safe margin, depending on the width of
// the transition band of the low pass filter
#define FFT_MIN_SAMPLE_RATE (DSP_BANDWIDTH * 2 / USABLE_BW_FACTOR)
// Minimum allowed span for the FTT
#define FFT_MIN_SPAN 8192
// Maximum allowed span for the FTT
#define FFT_MAX_SPAN 1048576

// If the desired span is higher than the maximum bandwidth that can be computed
// using a single FFT, we use multiple slices. Max number of slices (FFTs)
// calculated to fill the span
#define FFT_MAX_SLICES 4

// Number of taps of the low pass filter for decimation
#define FFT_LPF_FIR_FILTER_NTAPS 51

// Group delay (in number of blocks) of the FIR filter
#define FFT_LPF_FIR_FILTER_DELAY_BLOCKS 1

#define FFT_LPF_IIR_FILTER_NCOEFFS 8

// Referred to the radio frontend
#define FFT_MIN_DB -145
#define FFT_MAX_DB -30

// Threshold above noise floor to take a signal into account
#define FFT_SIGNAL_THRESHOLD_DB 10

#define FFT_TYPE_Q31 2
#define FFT_TYPE_FLOAT 3

#define FFT_TYPE FFT_TYPE_FLOAT

#define FFT_FIFO_SIZE_MIN ((FFT_N + FFT_LPF_FIR_FILTER_DELAY_BLOCKS * DSP_BLOCK) * MAX_DECIMATION_FACTOR)
#define FFT_FIFO_SIZE (((FFT_N + FFT_LPF_FIR_FILTER_DELAY_BLOCKS * DSP_BLOCK) * MAX_DECIMATION_FACTOR))
#define FFT_FIFO_SIZE_LOG2 ((size_t)ceil(log(FFT_FIFO_SIZE)))

enum FFT_WINDOW_TYPES { FFT_WINDOW_NONE, FFT_WINDOW_HAMMING };

enum FFT_VIEW_MODE { FFT_VIEW_SPECTRUM, FFT_VIEW_TIME_DOMAIN };

#define MIN_FFT_REFRESH_PERIOD 33

typedef struct {

    uint8_t max_slices = FFT_MAX_SLICES;
    uint32_t span = 750000;
    uint32_t bw = DSP_BANDWIDTH; // Bandwidth of interest of the FFT. Usable bandwidth.
    int16_t min_db = -130;
    int16_t max_db = -75;
    // bool min_db_auto = false;
    // bool show_noise_floor = true;
    // int resolution_bits = 16;
    int32_t maxAmpl = (2 / 3.3) * MAX_ADC_VALUE; // 2V peak to peak considering inputs to
                                                 // filters are biased at 1.1V
    uint8_t view_mode = FFT_VIEW_SPECTRUM;
    bool view_IQBalance = false;
    // When using CMX397 IC as quadrature demodulator, IQ balance is very good.
    // However, after the LPF block, phases may get unbalanced
    bool enable_iq_balance = true;
    float smooth_factor = 0.4; // 1: Minimum smooth

    uint8_t conversion_time_us = 3; // Conversion time of the ADCs
    bool enabled = true;
    uint8_t refresh_period_ms = 33; // Aim for 30 fps
    uint16_t waterfall_pixels_per_second = 1000 / (FFT_WATERFALL_MIN_REFRESH_PERIOD_MS + 10);

    int16_t DCOffset_I = 0;
    int16_t DCOffset_Q = 0;

    complex_t_f32 iq_balance_meanZ[FFT_N];
    float32_t iq_balance_precZ[FFT_N];

    // Period of the IQ balance estimator. 0 to disable estimation
    uint8_t iq_balance_estimate_period_ms = 10;

    bool removeDC = true;

    uint8_t window = FFT_WINDOW_HAMMING;
    uint32_t sample_rate = 200000;

    // Max sample frequency, determined by the ADC's capabilities and the
    // processing time, if we are processing the samples in parallel with the
    // adquisition (with double buffering and DMA) We should have enough time to
    // process the samples before the buffer gets overwritten by the running DMA
    // burst
    uint32_t max_sample_rate = ADC_MAX_SAMPLE_RATE;

    // Max sample frequency in DSP mode. When doing DSP, we spent more time
    // processing the ADC buffer in real time, so the sample frequency is even
    // more constrained. If we'd have enough processing power, ideally,
    // dsp_max_sample_rate and max_sample_rate would be the same. Currently, the FFT
    // has to be reconfigured when doing real time DSP (see
    // dsp_set_real_time function)
    uint32_t dsp_max_sample_rate = (ADC_MAX_SAMPLE_RATE / 2);

    // Min sample frequency, determined by the bandwidth of the ADC's low pass
    // filters Must be twice the bandwidth of interest plus the length of the
    // filter transition band
    uint32_t min_sample_rate = FFT_MIN_SAMPLE_RATE;

    uint8_t max_decimation_factor = MAX_DECIMATION_FACTOR;

    FFT_SPECTRUM_STYLE spectrum_style = FFT_SPECTRUM_STYLE_LINE_FILL;
    uint16_t spectrum_line_color = C565_CYAN;
    uint16_t spectrum_fill_color = C565_GREENYELLOW;
} st_fft_config;

#endif // TRX_FRONTEND_FFT_TYPES_H
