//
// Created by Angel Dust on 16/04/2021.
//

#ifndef TRX_FRONTEND_DSP_PROCESSOR_H
#define TRX_FRONTEND_DSP_PROCESSOR_H

#include "buffer.hpp"
#include "dsp.h"
#include "task.h"
#include "status.h"

class DspProcessor : public Task {

  public:
    virtual void work(const buffer_t<complex_t> *buffer) = 0;

    bool start() override {
        // LOG("DspProcessor START\n");
        this->reset();
        this->status.status = DSP_STATUS_RUNNING;
        this->status.start_ms = HAL_GetTick();
        return true;
    }

    void stop() override {
        // LOG("DspProcessor STOP\n");
        this->status.status = DSP_STATUS_STOPPED;
        this->status.stop_ms = HAL_GetTick();
    }

    void reset() override {
        this->status.status = DSP_STATUS_STOPPED;
        this->status.stop_ms = HAL_GetTick();
        this->status.reset();
    }

  private:
    void work() override{};
};

#endif // TRX_FRONTEND_DSP_PROCESSOR_H
