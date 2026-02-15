//
// Created by Angel Dust on 16/04/2021.
//

#include "dsp_replay_processor.h"

#include "dsp/dsp_buffers.h"
#include "dsp/dsp_common.h"
#include "config.h"
#include "FIFO.h"
#include "dsp/fft/fft_params.h"
#include "stm32f4xx_hal.h"

//#if DSP_REPLAY_DEBUG
#include "dsp/blocks/signal_generator.h"

//#endif

void DspReplayProcessor::work(const buffer_t<adc_type> *buffer) {

    if (this->info.status != DSP_STATUS_RUNNING) {
        memset((char *)buffer->p, 0, buffer->count << 1);
        return;
    }

    char *p;

    volatile uint16_t bytesToRead = this->info.block_size_bytes;

    uint32_t av = output_stream.available(&p);

    if (av >= bytesToRead) {

        adc_type *out_p = (adc_type *)buffer->p;

        //  GPIOD->BSRR = GPIO_PIN_9;
        for (size_t i = 0; i < buffer->count; i += 2) {

            out_p[i] = ((adc_type *)p)[i];

            if (this->info.n_channels == 2) {
                out_p[i + 1] = ((adc_type *)p)[i + 1];
            } else {
                out_p[i + 1] = 0;
            }
        }
        //  GPIOD->BSRR = GPIO_PIN_9 << 16;
        // auto t = HAL_GetTick();
        // static int last_t;
        // if (t - last_t > 100) {
        //     last_t = t;
        //     LOG_RAW("I:");
        //     for (size_t i = 0; i < buffer->count; i += 2) {
        //         LOG_RAW("%d,", out_p[i]);
        //     }
        //     LOG_RAW("\nQ:");
        //     for (size_t i = 1; i < buffer->count; i += 2) {
        //         LOG_RAW("%d,", out_p[i]);
        //     }
        //     LOG_RAW("\n", 0);
        // }

        output_stream.consume(bytesToRead, &p);
        this->info.processed_blocks++;

    } else {
        if (!output_stream.is_closed()) {
            this->info.fifo_underruns++;
        }

        memset((char *)buffer->p, 0, buffer->count << 1);
    }
}
