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
    virtual void work(const buffer_t<int16_t> *buffer) = 0;

    bool start() override {
        LOG("Starting %s processor\n", get_name());
        this->reset();
        this->status.status = DSP_STATUS_RUNNING;
        this->status.start_ms = HAL_GetTick();
        return true;
    }

    void stop() override {
        LOG("Stoppinng %s processor\n", get_name());
        this->status.status = DSP_STATUS_STOPPED;
        this->status.stop_ms = HAL_GetTick();
    }

    void reset() override {
        LOG("Resetting %s processor\n", get_name());
        this->status.status = DSP_STATUS_STOPPED;
        this->status.stop_ms = HAL_GetTick();
        this->status.reset();
    }

  private:
    void work() override{};
};

#endif // TRX_FRONTEND_DSP_PROCESSOR_H
