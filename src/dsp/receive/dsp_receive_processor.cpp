//
// Created by Angel Dust on 04/04/2025.
//

#include "dsp_receive_processor.h"
#include "dsp/dsp_buffers.h"
#include "dsp/dsp_common.h"
#include "dsp/blocks/signal_generator.h"
#include "types.h"
#include "printf.h"

void DspReceiveProcessor::work(const buffer_t<adc_type> *buffer) {

    if (status.status != DSP_STATUS_RUNNING) {
        return;
    }

    uint16_t *p;

    // TODO: Find some other way of making this processor know whether is reading or writing
    if (buffer->p == adc_buffer_1.p || buffer->p == adc_buffer_2.p) {
        uint16_t block_size_bytes = buffer->size_bytes;

        // GPIOD->BSRR |= GPIO_PIN_9;
        uint32_t free = input_stream.free((char **)&p);

        if (free >= block_size_bytes) {
            status.processed_blocks++;
            input_stream.write_block((char *)buffer->p, buffer->size_bytes);
        } else {
            status.fifo_overruns++;
        }
        // GPIOD->BSRR |= GPIO_PIN_9 << 16;
    } else {
        uint16_t block_size_bytes = buffer->size_bytes / 2;

        //  GPIOD->BSRR |= GPIO_PIN_9;
        uint32_t av = output_stream.available((char **)&p);

        if (av >= block_size_bytes) {

            int16_t *out_p = (int16_t *)buffer->p;

            for (size_t i = 0; i < buffer->count / 2; i++) {
                out_p[i * 2] = (((uint16_t *)p)[i] + config.hw.dac_offset) * dsp::dsp_params->gain;
                // out_p[i + 1] = ((uint16_t *)p)[i + 1] + config.hw.dac_offset;
            }

            output_stream.consume(block_size_bytes, (char **)&p);

        } else if (status.processed_blocks) {
            // Underruns will surely happen at the start of the process
            status.fifo_underruns++;
        }
        // GPIOD->BSRR |= GPIO_PIN_9 << 16;
    }

    // Error rate
}
