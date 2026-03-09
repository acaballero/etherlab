//
// OOK (On-Off Keying) Transmit Task
//

#ifndef TRX_FRONTEND_OOK_TASK_H
#define TRX_FRONTEND_OOK_TASK_H

#include <memory>
#include "dsp/ook/dsp_ook_processor.h"
#include "dsp/task.h"
#include "types.h"

class OOKTask : public Task {

  public:
    using Task::Task;

    OOKTask() : Task() {
        info.id = 101; // Custom task ID (not in the DSP_TASK_ID enum, like APRS uses 100)
    }

    void work() override;

    bool start_impl() override;

    void stop() override;

    DspOOKProcessor *get_ook_processor() {
        return (DspOOKProcessor *)get_processor();
    }

  protected:
    std::unique_ptr<DspProcessor> create_processor() override {
        return std::make_unique<DspOOKProcessor>();
    }
};

#endif // TRX_FRONTEND_OOK_TASK_H
