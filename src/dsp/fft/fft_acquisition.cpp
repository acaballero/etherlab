//
// Created by Angel Dust on 23/02/2026.
//
#include "fft_acquisition.h"
#include "dsp/dsp_common.h"

// Buffers in CCM RAM
CCM_SECTION static complex_t pipeline_raw_buff[FFT_FIFO_SIZE];
CCM_SECTION static complex_t_f32 pipeline_decimated_buff[FFT_DECIMATED_FIFO_SIZE];

FFTAcquisition fft_acquisition((char *)pipeline_raw_buff, FFT_FIFO_SIZE * sizeof(complex_t), (char *)pipeline_decimated_buff,
                               FFT_DECIMATED_FIFO_SIZE * sizeof(complex_t_f32));

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
    while (raw.available(&raw_p) >= raw_block_bytes) {

        const uint16_t decimated_block_size = DSP_BLOCK / decimation_factor;
        const uint32_t decimated_block_bytes = decimated_block_size * sizeof(complex_t_f32);

        // If the decimated FIFO is full, close the raw FIFO
        if (decimated.free() < decimated_block_bytes) {
            raw.close();
            return;
        }

        if (decimation_factor > 1) {

            float32_t signal[DSP_BLOCK * 2];
            buffer_t<float32_t> src(signal, DSP_BLOCK * 2);
            dsp::s16_to_f32((adc_type *)raw_p, signal, DSP_BLOCK * 2);

            complex_t_f32 dec_out[DSP_BLOCK]; // DSP_BLOCK >= decimated_block_size always
            buffer_t<float32_t> dst((float32_t *)dec_out, decimated_block_size);
            dst.decimated_size_bytes = decimated_block_size;

            dec_i->decimate(src, dst, 0, 2, 1); // Invert I/Q
            dec_q->decimate(src, dst, 1, 2, 0);

            // Skip filter group delay blocks
            const uint32_t delay_blocks = FFT_LPF_FIR_FILTER_DELAY_BLOCKS * decimation_factor;
            if (raw_blocks_processed >= delay_blocks) {
                decimated.write_block((char *)dec_out, decimated_block_bytes);
            }

        } else {
            // No decimation: s16 → float, write directly
            complex_t_f32 out[DSP_BLOCK];
            complex_t *in = (complex_t *)raw_p;
            for (int i = 0; i < DSP_BLOCK; i++) {
                out[i].r = (float32_t)in[i].r;
                out[i].i = (float32_t)in[i].i;
            }
            decimated.write_block((char *)out, DSP_BLOCK * sizeof(complex_t_f32));
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
    while (decimated.available(&data) < needed_bytes) {

        process(decimation_factor);
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
