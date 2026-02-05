//
// Created by Angel Dust on 16/04/2021.
//

#ifndef TRX_FRONTEND_REPLAY_TASK_H
#define TRX_FRONTEND_REPLAY_TASK_H

#include <memory>
#include "dsp/replay/dsp_replay_processor.h"
#include "dsp/task.h"
#include "ui/sd_filepicker_menu.h"
#include "io/wav.h"

class ReplayTask : public Task {

  public:
    using Task::Task;

    void work() override;

    bool start_impl() override;

    void stop() override;

    void setFile(File *);

    bool getLoop() const;

    void setLoop(bool loop);

  protected:
    std::unique_ptr<DspProcessor> create_processor() override {
        return std::make_unique<DspReplayProcessor>();
    }

  private:
    bool loop;
    File *m_file;
};

#endif // TRX_FRONTEND_REPLAY_TASK_H
