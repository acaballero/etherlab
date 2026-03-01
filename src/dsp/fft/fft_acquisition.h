//
// Created by Angel Dust on 23/02/2026.
//
#pragma once

#include "hw/stm32.h"
#include "FIFO.h"
#include "dsp/dsp_common.h"
#include "dsp/fft/fft_types.h"
#include "dsp/decimation/dsp_fir_decimator_float.h"
#include "os/task_manager.h"

/*
 * Owns the two-stage FFT acquisition pipeline:
 *   interrupt → raw FIFO → [drain/decimate] → decimated FIFO → FFT processing
 *
 * A raw FIFO overrun marks the raw FIFO as closed (invalid). drain() detects
 * this and resets the whole pipeline before resuming.
 */
struct FFTAcquisition {

    static constexpr uint8_t MAX_DECIMATORS = 2;
    static constexpr uint8_t MAX_STAGE1_FACTOR = 8;

    FIFO raw;
    FIFO decimated;

    std::unique_ptr<DspDecimator<float32_t>> decimators[MAX_DECIMATORS];

    uint8_t n_decimators = 0;
    uint8_t decimation_factor = 1;

    uint32_t raw_blocks_processed = 0;

    uint32_t overruns_min = 0;

    FFTAcquisition(char *raw_buf, uint32_t raw_size, char *dec_buf, uint32_t dec_size) : raw(raw_buf, raw_size), decimated(dec_buf, dec_size) {

        // Refresh clock
        os::periodic_task task{1000, [this](void) {
                                   overruns_min = overruns;
                                   overruns = 0;
                               }};

        os::task_manager.add(&task);
    }

    /*
     * Configure the decimation chain.
     * Allocates / reconfigures decimator objects as needed.
     * Calls reset() internally.
     */
    bool config(uint32_t input_rate, uint32_t bw, uint8_t factor);

    void feed(const complex_t *block, uint32_t size_bytes);
    void process(uint8_t decimation_factor);
    bool consume(complex_t_f32 *dst, uint16_t n, uint8_t decimation_factor, uint32_t timeout_ms = 200);

    void reset() {
        raw.reset();
        decimated.reset();
        raw_blocks_processed = 0;
        for (uint8_t i = 0; i < n_decimators; i++) {
            if (decimators[i]) {
                // (DspFIRDecimatorFloat<>)decimators[i]->clear_state();
            }
        }
    }

  private:
    // Intermediate buffer between stage 0 and stage 1 output (separated I and Q).
    // Max size: DSP_BLOCK / MAX_STAGE1_FACTOR samples per channel.
    static constexpr uint16_t INTER_SAMPLES = DSP_BLOCK / MAX_STAGE1_FACTOR;
    float32_t inter_i[DSP_BLOCK / 2];
    float32_t inter_q[DSP_BLOCK / 2];

    // Float buffers
    float32_t src_i[DSP_BLOCK];
    float32_t src_q[DSP_BLOCK];

    uint32_t overruns = 0;
};

extern FFTAcquisition fft_acquisition;
