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

    bool paint_callback() override;
    void setProcessorStatus(st_dsp_params *status);
    void setTaskStatus(st_dsp_params *status);

  protected:
    void before_paint() override;
    st_dsp_params *processor_status;
    st_dsp_params *task_status;
};

#endif // TRX_FRONTEND_REPLAY_WIDGET_H
