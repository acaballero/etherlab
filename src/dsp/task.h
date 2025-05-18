//
// Created by Angel Dust on 16/04/2021.
//

#ifndef TRX_FRONTEND_TASK_H
#define TRX_FRONTEND_TASK_H

#include "dsp_common.h"

class Task {

  public:
    virtual ~Task() = default;
    virtual bool start();
    virtual void stop();
    virtual void work() = 0;
    void reset();
    void halt(DSP_ERROR);
    st_dsp_status status;

  protected:
    void (*on_success)();
    void (*on_error)(DSP_ERROR);
};

#endif // TRX_FRONTEND_TASK_H
