//
// Created by Angel Dust on 16/04/2021.
//

#include "dsp_replay_processor.h"

#include "dsp/dsp_buffers.h"
#include "dsp/dsp_common.h"
#include "config.h"
#include "FIFO.h"

#if DSP_REPLAY_DEBUG
#include "dsp/signal_generator.h"
SignalGenerator sig_gen(1000, 346666);
#endif

void DspReplayProcessor::work(const buffer_t<adc_type> *buffer) {

    if (this->status.status != DSP_STATUS_RUNNING) {
        memset((char *)buffer->p, 0, buffer->count << 1);
        return;
    }

    // The samples are stored in a particular sample rate but, if we are processing
    // them in a wider bandwidth FFT, or our output sample rate to the DAC or transceiver
    // is higher, we will need to interpolate/oversample.
    // If we only need to interpolate for the FFT, it doesn't need to be done in real-time but,
    // if we need to output the result to a DAC, we will need to make it here
    // TODO: Use better interpolation, at least in the FFT code. Otherwise, the spectrum will show harmonics at the saved sample rate

    this->status.processed_blocks++;

    char *p;

    // e.g. if interpolation/decimation factor is 4, we read 4 times fewer bytes that the DAC block size
    volatile uint16_t bytesToRead = this->status.decimated_block_size_bytes;

    uint32_t av = output_stream.available(&p);

    if (av >= bytesToRead) {

        adc_type *out_p = (adc_type *)buffer->p;

        // Naive interpolation
        // Output channel number is always 2
        // Expects buffer to be complex interleaved
        for (size_t i = 0, j = 0; i < buffer->count; i += 2) {

            if ((i >> 1) & (this->status.decimation_factor - 1)) {
                // if (d > 0) {
                out_p[i] = out_p[i - 2];
                //  if (this->status.n_channels==2) {
                out_p[i + 1] = out_p[i - 1];
                //  }
                // d--;

            } else {
                out_p[i] = ((adc_type *)p)[j];
                //           LOG("%d,", out_p[i]);

                if (this->status.n_channels == 2) {
                    out_p[i + 1] = ((adc_type *)p)[j + 1];
                    j++;
                } else {
                    out_p[i + 1] = 0;
                }
                j++;
                //   d = this->status.decimation_factor-1;
            }
        }

        output_stream.consume(bytesToRead, &p);

        // LOG_RAW("I:");
        // for (size_t i = 0; i < buffer->count; i += 2) {
        //     LOG_RAW("%d,", out_p[i]);
        // }
        // LOG_RAW("\nQ:");
        // for (size_t i = 1; i < buffer->count; i += 2) {
        //     LOG_RAW("%d,", ((adc_type *)p)[i]);
        // }
        // LOG_RAW("\n", 0);

    } else {
        if (!output_stream.is_closed()) {
            this->status.fifo_underruns++;
        }

        memset((char *)buffer->p, 0, buffer->count << 1);
    }
}
