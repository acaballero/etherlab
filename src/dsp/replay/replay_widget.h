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

    void setFileInfo(FILINFO finfo);

    void setProcessorStatus(st_dspStatus *status);

    void setTaskStatus(st_dspStatus *status);

    void setShowActions(bool b) { show_actions = b; }

    bool getShowActions() { return show_actions; }

protected:

    void do_paint() override;

    bool show_actions = false;
    st_dspStatus *processor_status;
    st_dspStatus *task_status;
    WaveInfo wi;
    FILINFO finfo;
};

#endif //TRX_FRONTEND_REPLAY_WIDGET_H
