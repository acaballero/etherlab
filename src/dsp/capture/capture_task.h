//
// Created by Angel Dust on 16/04/2021.
//

#ifndef TRX_FRONTEND_CAPTURE_TASK_H
#define TRX_FRONTEND_CAPTURE_TASK_H

#include <bits/unique_ptr.h>
#include "../task.h"
#include "fatfs/fatfs.h"
#include "dsp/capture/dsp_capture_processor.h"
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
#include "dsp/decimation/dsp_fir_decimator_float.h"
#include "../../../lib/utils/utils.hpp"
#include "io/file_factory.h"
#include "dsp/blocks/dc_block.h"

class CaptureTask : public Task {
  public:
    using Task::Task;

    void work() override;

    bool start_impl() override;

    void stop() override;

    void setFile(std::unique_ptr<File> file);

    File *getFile();

  protected:
    std::unique_ptr<DspProcessor> create_processor() override {
        return std::make_unique<DspCaptureProcessor>();
    }

  private:
    std::unique_ptr<File> file;

    void init();
};

#endif // TRX_FRONTEND_CAPTURE_TASK_H
