//
// Created by Angel Dust on 16/04/2021.
//

#ifndef TRX_FRONTEND_CAPTURE_TASK_H
#define TRX_FRONTEND_CAPTURE_TASK_H

#include <bits/unique_ptr.h>
#include "../task.h"
#include "fatfs/fatfs.h"
#include "io/wav.h"
#include <memory>
#include "dsp/dsp_common.h"
#include "ui/view.h"
#include "config.h"
#include "fatfs/fatfs.h"
#include "hw/stm32f4xx/timers.h"
#include "status.h"
#include "ui/lcd.h"
#include "dsp/dsp_buffers.h"
#include "dsp/decimation/dsp_decimators.h"
#include "../../../lib/utils/utils.hpp"
#include "io/file_factory.h"

class CaptureTask : public Task {
  public:
    CaptureTask(void (*onSucess)(), void (*onError)(DSP_ERROR));

    void work() override;

    bool start() override;

    void stop() override;

    void configureDsp();

    void setFile(std::unique_ptr<File> file);
    File *getFile();

  private:
    std::unique_ptr<File> file;
};

#endif // TRX_FRONTEND_CAPTURE_TASK_H
