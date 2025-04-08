//
// Created by Angel Dust on 16/04/2021.
//

#include "hw/stm32.h"
#include "task.h"
#include "dsp_buffers.h"

void Task::reset() {

    // TODO: I don't remember why this was required for, but now there are two streams, so probably that's not what I want
    output_stream.reset();

    this->status.fifo_overruns = 0;
    this->status.fifo_underruns = 0;
    this->status.processed_blocks = 0;
    // this->status.decimation_factor = 1;
    // this->status.block_size_bytes = dsp_temp_buf.size_bytes;
    // this->status.decimated_block_size = dsp_temp_buf.decimated_size_bytes;
    // this->status.bits_per_sample = 0;
    // this->status.n_channels = 2;
    this->status.error = DSP_ERR_NONE;
    this->status.stop_ms = 0;
    this->status.last_error_ms = 0;
    this->status.start_ms = HAL_GetTick();
}

void Task::halt(DSP_ERROR e) {
    this->status.error = e;
    this->stop();
}

void Task::start() {
    this->status.status = DSP_STATUS_RUNNING;
    this->reset();
    this->status.start_ms = HAL_GetTick();
}

void Task::stop() {

    this->status.status = DSP_STATUS_STOPPED;
    this->status.stop_ms = HAL_GetTick();

    if (this->status.error) {
        if (this->on_error) {
            this->on_error(this->status.error);
        }
    } else {
        if (this->on_success) {
            this->on_success();
        }
    }
}
