//
// Created by Angel Dust on 17/04/2021.
//

#ifndef TRX_FRONTEND_REPLAY_WIDGET_H
#define TRX_FRONTEND_REPLAY_WIDGET_H

#include "../../ui/widget.h"
#include "../dsp_common.h"
#include "fatfs/fatfs.h"
#include "io/wav.h"

class ReplayWidget : public Widget {
  public:
    using Widget::Widget;

    void paint_callback() override;

    void setWaveInfo(WaveInfo wi);

    void setProcessorStatus(st_dsp_status *status);

    void setTaskStatus(st_dsp_status *status);

    void setShowActions(bool b) {
        show_actions = b;
    }

    bool getShowActions() {
        return show_actions;
    }

  protected:
    void before_paint() override;

    bool show_actions = false;
    st_dsp_status *processor_status;
    st_dsp_status *task_status;
    WaveInfo wi;
};

#endif // TRX_FRONTEND_REPLAY_WIDGET_H
