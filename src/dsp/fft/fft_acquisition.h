//
// Created by Angel Dust on 23/02/2026.
//
#pragma once

#include "hw/stm32.h"
#include "FIFO.h"
#include "dsp/dsp_common.h"
#include "dsp/fft/fft_types.h"
#include "dsp/decimation/dsp_fir_decimator_float.h"

/*
 * Owns the two-stage FFT acquisition pipeline:
 *   interrupt → raw FIFO → [drain/decimate] → decimated FIFO → FFT processing
 *
 * A raw FIFO overrun marks the raw FIFO as closed (invalid). drain() detects
 * this and resets the whole pipeline before resuming.
 */
struct FFTAcquisition {

    FIFO raw;
    FIFO decimated;

    DspFIRDecimatorFloat<FFT_LPF_FIR_FILTER_NTAPS> *dec_i = nullptr;
    DspFIRDecimatorFloat<FFT_LPF_FIR_FILTER_NTAPS> *dec_q = nullptr;

    uint32_t raw_blocks_processed = 0;
    uint32_t overruns = 0;

    FFTAcquisition(char *raw_buf, uint32_t raw_size, char *dec_buf, uint32_t dec_size) : raw(raw_buf, raw_size), decimated(dec_buf, dec_size) {
    }

    void init(DspFIRDecimatorFloat<FFT_LPF_FIR_FILTER_NTAPS> &i, DspFIRDecimatorFloat<FFT_LPF_FIR_FILTER_NTAPS> &q) {
        dec_i = &i;
        dec_q = &q;
    }

    void feed(const complex_t *block, uint32_t size_bytes);
    void process(uint8_t decimation_factor);
    bool consume(complex_t_f32 *dst, uint16_t n, uint8_t decimation_factor, uint32_t timeout_ms = 200);

    void reset() {
        raw.reset();
        decimated.reset();
        raw_blocks_processed = 0;

        if (dec_i) {
            dec_i->clear_state();
        }
        if (dec_q) {
            dec_q->clear_state();
        }
    }
};

extern FFTAcquisition fft_acquisition;
