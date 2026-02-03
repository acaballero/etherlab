//
// Created by Angel Dust on 16/04/2021.
//

#include "dsp/buffer.hpp"
#include "dsp/dsp_buffers.h"
#include "dsp/dsp_common.h"
#include "dsp/dsp_processor.h"
#include "dsp_capture_processor.h"
#include "hw/stm32f4xx/clocks.h"

void DspCaptureProcessor::work(const buffer_t<adc_type> *buffer) {

    // This processor does decimation and some heavy processing, which is usually done in the related task. However,
    // decimating here saves memory since the capture task needs to process large blocks for the SD card writes to be efficient, but their size
    // would be multiplied by the decimation factor

    if (info.status != DSP_STATUS_RUNNING) {
        return;
    }

    GPIOD->BSRR |= GPIO_PIN_9;

    auto p = buffer->p;
    size_t size = buffer->count;

    if (info.decimation_factor > 1) {
        // TODO: Perhaps converting the stream to float for decimation and back is not worth it here. At least using the unzipped version of the decimator
        // does not make much sense for just one decimation step (the f32 interleaved buffer could be wrapped in buffer_t<complex_t_f32> bb = {(complex_t_f32
        // *)bi2_p, DSP_BLOCK} and then decimated. But again, just an integer decimator would do.

        dsp::s16_to_f32((const adc_type *)p, bi1_p, size);
        dsp::unzip_f32((const float32_t *)bi1_p, bi2_p, bq2_p, size / 2);

        buffer_t<float32_t> b1{bi2_p, size / 2, 0, COMPLEX_SEQUENTIAL};
        buffer_t<float32_t> b2{bi1_p, (size_t)info.decimated_block_size / 2, 0, COMPLEX_SEQUENTIAL};

        decimator.decimate(b1, b2);

        dsp::zip_f32(bi1_p, bi1_p + info.decimated_block_size, (float32_t *)bi2_p, info.decimated_block_size << 1);

        dsp::f32_to_s16((const float32_t *)bi2_p, (adc_type *)p, info.decimated_block_size << 1);
    }

    buffer_t<adc_type> bb = {(adc_type *)p, static_cast<size_t>(info.decimated_block_size << 1)};
    dc_blocker_i.filter(bb, info.n_channels, 0);
    dc_blocker_q.filter(bb, info.n_channels, 1);

    this->info.processed_blocks++;

    FIFO_ERROR err = input_stream.write_block((char *)buffer->p, info.decimated_block_size_bytes);

    if (err != FIFO_ERROR_NONE) {
        this->info.fifo_overruns++;
    }

    GPIOD->BSRR |= GPIO_PIN_9 << 16;
}
bool DspCaptureProcessor::start() {

    bool ok = true;
    if (info.decimation_factor > 1) {
        ok = decimator.config(info.sample_rate, info.bandwidth, info.decimation_factor);
    }
    if (ok) {
        DspProcessor::start();
    }
    return ok;
}
