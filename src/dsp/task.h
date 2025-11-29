//
// Created by Angel Dust on 16/04/2021.
//

#ifndef TRX_FRONTEND_TASK_H
#define TRX_FRONTEND_TASK_H

#include "dsp_common.h"

class Task {

  public:
    Task(){};
    Task(void (*onSucess)(), void (*onError)(DSP_ERROR));
    virtual ~Task() = default;
    virtual const char *get_name() {
        return "-";
    };
    virtual bool start();
    virtual void stop();
    virtual void work() = 0;
    virtual void reset();
    void halt(DSP_ERROR);
    st_dsp_params status;

    // Callback for the first processed block
    std::function<void()> on_first_block{};

  protected:
    void (*on_success)();
    void (*on_error)(DSP_ERROR);
};

#endif // TRX_FRONTEND_TASK_H
