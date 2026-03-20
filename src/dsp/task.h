//
// Created by Angel Dust on 16/04/2021.
//

#ifndef TRX_FRONTEND_TASK_H
#define TRX_FRONTEND_TASK_H

#include "dsp/dsp_processor.h"
#include "dsp_common.h"
#include <memory>

class Task {

  public:
    Task(){};

    virtual ~Task() {
        if (processor) {
            processor.reset();
        }
    };

    virtual const char *get_name() {
        return "-";
    };
    virtual bool start();
    virtual void stop();
    virtual void work() = 0;
    virtual void reset();
    void abort(DSP_ERROR);
    st_dsp_params info{};

    // Get runtime info
    virtual st_dsp_params *get_info() {
        // FIXME: The authoritative info is from the processor or the task depending on who carries the relevant information
        // regarding overruns/underruns. This is  ugly
        if (!processor) {
            processor = create_processor();
        }
        return info.direction != DSP_DIRECTION_IN && processor ? &processor->info : &info;
    }

    DspProcessor *get_processor() {
        if (!processor) {
            processor = create_processor();
        }
        return processor.get();
    }

    // Callback for the first processed block
    std::function<void()> on_first_block{};

    Signal on_event;

  protected:
    // Derived task-specific initialization. Called before processor startup
    virtual bool start_impl() = 0;

    std::unique_ptr<DspProcessor> processor;

  private:
    // Common processor startup logicd
    bool start_processor();

    // Creates the processor for this task (nullptr if task has no processor)
    virtual std::unique_ptr<DspProcessor> create_processor() {
        return nullptr;
    }
};

#endif // TRX_FRONTEND_TASK_H
