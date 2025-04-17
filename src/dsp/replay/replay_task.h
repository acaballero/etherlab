//
// Created by Angel Dust on 16/04/2021.
//

#ifndef TRX_FRONTEND_REPLAY_TASK_H
#define TRX_FRONTEND_REPLAY_TASK_H

#include <memory>
#include "dsp/task.h"
#include "ui/sd_filepicker_menu.h"
#include "io/wav.h"

class ReplayTask : public Task {

  public:
    ReplayTask(void (*onSucess)(), void (*onError)(DSP_ERROR));

    void work() override;

    bool start() override;

    void stop() override;

    void setFile(std::unique_ptr<File>);

    bool getLoop() const;

    void setLoop(bool loop);

  private:
    bool loop;
    std::unique_ptr<File> m_file;
};

#endif // TRX_FRONTEND_REPLAY_TASK_H
