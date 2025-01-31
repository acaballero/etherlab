//
// Created by Angel Dust on 17/04/2021.
//

#ifndef TRX_FRONTEND_CAPTURE_WIDGET_H
#define TRX_FRONTEND_CAPTURE_WIDGET_H

#include "../../ui/widget.h"
#include "../dsp_common.h"
#include "dsp/dsp_buffers.h"
#include "../../../lib/utils/utils.hpp"

class CaptureWidget : public Widget {
  public:
    using Widget::Widget;

    void paint_callback() override;
    void setProcessorStatus(st_dspStatus *status);
    void setTaskStatus(st_dspStatus *status);

  protected:
    void before_paint() override;
    st_dspStatus *processor_status;
    st_dspStatus *task_status;
};

#endif // TRX_FRONTEND_REPLAY_WIDGET_H
