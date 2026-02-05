//
// Created by Angel Dust on 16/04/2021.
//

#ifndef TRX_FRONTEND_DSP_TASK_H
#define TRX_FRONTEND_DSP_TASK_H

#include "buffer.hpp"
#include "status.h"

class DspProcessor {

  public:
    DspProcessor(){};
    DspProcessor(void (*onSucess)(), void (*onError)(DSP_ERROR));
    virtual ~DspProcessor() = default;

    virtual void work(const buffer_t<int16_t> *buffer) = 0;

    virtual const char *get_name() {
        return "-";
    };

    virtual bool start() {
        LOG("Starting %s processor\n", get_name());
        this->reset();
        this->info.status = DSP_STATUS_RUNNING;
        this->info.start_ms = HAL_GetTick();
        return true;
    }

    virtual void stop() {
        LOG("Stoppinng %s processor\n", get_name());
        this->info.status = DSP_STATUS_STOPPED;
        this->info.stop_ms = HAL_GetTick();
    }

    virtual void reset() {
        LOG("Resetting %s processor\n", get_name());
        this->info.status = DSP_STATUS_STOPPED;
        this->info.stop_ms = HAL_GetTick();
        this->info.reset();
    }

    st_dsp_params info{};

  private:
    virtual void work(){};
};

#endif // TRX_FRONTEND_DSP_TASK_H
