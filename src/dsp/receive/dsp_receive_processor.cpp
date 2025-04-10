//
// Created by Angel Dust on 04/04/2025.
//

#include "dsp_receive_processor.h"
#include "dsp/dsp_buffers.h"
#include "dsp/dsp_common.h"
#include "dsp/blocks/signal_generator.h"
#include "types.h"
#include "printf.h"

void DspReceiveProcessor::work(const buffer_t<complex_t> *buffer) {

    uint16_t *p;
    uint16_t block_size_bytes = buffer->size_bytes;

    // TODO: Find some other way of making this processor know whether is reading or writing
    if (buffer->p == adc_buffer_1.p || buffer->p == adc_buffer_2.p) {

        uint32_t free = input_stream.free((char **)&p);

        if (free >= block_size_bytes) {

            status.processed_blocks++;

            uint16_t *in_p = (uint16_t *)buffer->p;

            for (size_t i = 0; i < buffer->count * 2; i += 2) {
                // TODO: Gain should be a generic and stackable block
                p[i] = ((uint16_t *)in_p)[i];
                //   p[i + 1] = ((uint16_t *)in_p)[i + 1];
            }

            input_stream.feed(block_size_bytes);

        } else {
            status.fifo_overruns++;
        }
    } else {

        uint32_t av = output_stream.available((char **)&p);

        if (av >= block_size_bytes) {

            int16_t *out_p = (int16_t *)buffer->p;

            for (size_t i = 0; i < buffer->count * 2; i += 2) {
                out_p[i] = ((uint16_t *)p)[i] + config.hw.dac_offset;
                //  out_p[i + 1] = ((uint16_t *)p)[i + 1] + config.hw.dac_offset;
            }

            output_stream.consume(block_size_bytes, (char **)&p);

        } else if (status.processed_blocks) {
            // Underruns will surely happen at the start of the process
            status.fifo_underruns++;
        }
    }

    // Error rate
}
