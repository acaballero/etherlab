//
// Created by Angel Dust on 16/04/2021.
//

#include "hw/stm32.h"
#include "task.h"
#include "dsp_buffers.h"

Task::Task(void (*onSucess)(), void (*onError)(DSP_ERROR)) {
    this->on_error = onError;
    this->on_success = onSucess;
}

void Task::reset() {
    status.reset();
}

void Task::halt(DSP_ERROR e) {
    this->status.error = e;
    this->stop();
}

bool Task::start() {
    this->status.status = DSP_STATUS_RUNNING;
    this->reset();
    this->status.start_ms = HAL_GetTick();
    return true;
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
