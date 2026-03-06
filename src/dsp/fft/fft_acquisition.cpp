//
// Created by Angel Dust on 23/02/2026.
//
#include "fft_acquisition.h"
#include "dsp/dsp_common.h"
#include "stm32f4xx_hal.h"

// Buffers in CCM RAM
CCM_SECTION static complex_t pipeline_raw_buff[FFT_FIFO_SIZE];
CCM_SECTION static complex_t_f32 pipeline_decimated_buff[FFT_DECIMATED_FIFO_SIZE];

FFTAcquisition fft_acquisition((char *)pipeline_raw_buff, FFT_FIFO_SIZE * sizeof(complex_t), (char *)pipeline_decimated_buff,
                               FFT_DECIMATED_FIFO_SIZE * sizeof(complex_t_f32));

bool FFTAcquisition::config(uint32_t input_rate, uint32_t bw, uint8_t factor) {

    decimation_factor = factor;
    n_decimators = 0;

    LOG("FFT acquisition config | input_rate: %d | bandwidth: %d | decimation factor: %d\n", input_rate, bw, factor);

    bool ok = true;

    if (factor <= 1) {
        // No decimation — no stages needed.
        reset();
        return true;
    }

    if (factor <= MAX_STAGE1_FACTOR) {
        // Single stage
        if (!decimators[0]) {
            decimators[0] = std::make_unique<DspFIRDecimatorFloat<FFT_LPF_FIR_FILTER_NTAPS>>();
        }

        ok = decimators[0]->config(input_rate, bw, factor);
        decimators[1].reset();
        n_decimators = 1;

    } else {
        // Two stages: stage 0 at ×MAX_STAGE1_FACTOR, stage 1 at ×(factor/MAX_STAGE1_FACTOR)
        uint32_t inter_rate = input_rate / MAX_STAGE1_FACTOR;
        uint8_t stage1_factor = factor / MAX_STAGE1_FACTOR;

        if (!decimators[0]) {
            decimators[0] = std::make_unique<DspFIRDecimatorFloat<FIR_DECIMATOR_1ST_HALFBAND_TAPS>>();
        }
        if (!decimators[1]) {
            decimators[1] = std::make_unique<DspFIRDecimatorFloat<FFT_LPF_FIR_FILTER_NTAPS>>();
        }

        ok = decimators[0]->config(input_rate, input_rate / MAX_DECIMATION_FACTOR / 2, MAX_STAGE1_FACTOR);
        ok &= decimators[1]->config(inter_rate, bw, stage1_factor);
        n_decimators = 2;
    }

    reset();
    return ok;
}

void FFTAcquisition::feed(const complex_t *block, uint32_t size_bytes) {
    if (raw.is_closed()) {
        return;
    }
    if (raw.write_block((char *)block, size_bytes) == FIFO_ERROR_OVERRUN) {
        overruns++;
    }
}

void FFTAcquisition::process(uint8_t decimation_factor) {

    const uint32_t raw_block_bytes = DSP_BLOCK * sizeof(complex_t);

    char *raw_p;
    char *dec_p;
    while (raw.available(&raw_p) >= raw_block_bytes) {

        const uint16_t out_samples = DSP_BLOCK / decimation_factor;
        const uint32_t out_bytes = out_samples * sizeof(complex_t_f32);
        const uint32_t delay_blocks = FFT_LPF_FIR_FILTER_DELAY_BLOCKS * decimation_factor;

        // If the decimated FIFO is full, close the raw FIFO
        if (decimated.free(&dec_p) < out_bytes) {
            raw.close();
            return;
        }

        complex_t_f32 *out = (complex_t_f32 *)dec_p;

        dsp::s16_unzip_f32((const adc_type *)raw_p, src_q, src_i,
                           DSP_BLOCK); // Note: Invert I/Q here (somehow they come inverted stil don't know why, but guess is the board paths or the ADC
                                       // channels that are swapped)

        if (n_decimators == 0) {
            // No decimation: convert s16 interleaved IQ to float
            // Unzipping is required to to invert the samples
            dsp::zip_f32(src_i, src_q, (float32_t *)out, DSP_BLOCK);
            decimated.feed(out_bytes);

        } else {

            float32_t *res_i, *res_q;

            if (n_decimators == 2) {
                decimators[0]->decimate(src_i, src_q, inter_i, inter_q, DSP_BLOCK);
                decimators[1]->decimate(inter_i, inter_q, src_i, src_q, INTER_SAMPLES);
                res_i = src_i;
                res_q = src_q;
            } else {
                decimators[0]->decimate(src_i, src_q, inter_i, inter_q, DSP_BLOCK);
                res_i = inter_i;
                res_q = inter_q;
            }

            if (raw_blocks_processed >= delay_blocks) {
                dsp::zip_f32(res_i, res_q, (float32_t *)out, out_samples);
                decimated.feed(out_bytes);
            }
        }

        __disable_irq(); // Prevents pre-emption by DMA interrupt
        raw.consume(raw_block_bytes, &raw_p);
        __enable_irq();
        raw_blocks_processed++;
    }
}

bool FFTAcquisition::consume(complex_t_f32 *dst, uint16_t n, uint8_t decimation_factor, uint32_t timeout_ms) {

    const uint32_t needed_bytes = n * sizeof(complex_t_f32);
    const uint64_t deadline = HAL_GetTick() + timeout_ms;
    char *data;
    while (decimated.available(&data) < needed_bytes) { // Note this should't be required and may hide the real-time processing not being fired
        __disable_irq();                                // Prevents race-conditions with ISR handlers
        process(decimation_factor);
        __enable_irq();
        if (HAL_GetTick() > deadline) {
            return false;
        }
    }

    memcpy(dst, data, needed_bytes);
    __disable_irq(); // Prevents race-conditions with ISR handlers
    decimated.consume(needed_bytes, &data);
    reset();
    __enable_irq();
    return true;
}
