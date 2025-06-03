//
// Created by Angel Dust on 16/04/2021.
//

#include <dsp/decimation/dsp_decimators.h>
#include "dsp/buffer.hpp"
#include "dsp/dsp_buffers.h"
#include "dsp_capture_processor.h"

void DspCaptureProcessor::work(const buffer_t<complex_t> *buffer) {

    this->status.processed_blocks++;

    // Wrap the complex_t buffer with an adc_type buffer
    buffer_t<adc_type> buff = {(adc_type *)buffer->p, DSP_BLOCK * 2};

    if (this->status.decimation_factor > 1) {
        IIRDecimator_I.decimate(buff, buff, 0, 2, this->status.n_channels);
        if (this->status.n_channels == 2) {
            IIRDecimator_Q.decimate(buff, buff, 1, 2, this->status.n_channels);
        }
    }

    buff.count = buff.count / this->status.decimation_factor;

#if !DSP_FS4_SHIFT
    dc_blocker_i.filter(buff, this->status.n_channels, 0);
    dc_blocker_q.filter(buff, this->status.n_channels, 1);
#endif

    FIFO_ERROR err = input_stream.writeBlock((char *)buff.p, this->status.decimated_block_size_bytes);

    if (err != FIFO_ERROR_NONE) {

        this->status.fifo_overruns++;

        if (this->status.fifo_overruns > 10) {
            this->status.error = DSP_ERR_FIFO_OVERRUN;
        }
    }
}
