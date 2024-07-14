//
// Created by Angel Dust on 17/04/2021.
//

#ifndef TRX_FRONTEND_SIGNAL_GENERATOR_WIDGET_H
#define TRX_FRONTEND_SIGNAL_GENERATOR_WIDGET_H

#include "../../ui/widget.h"
#include "../dsp_common.h"
#include "fatfs/fatfs.h"
#include "io/wav.h"

class SignalGeneratorWidget : public Widget {
public:
    using Widget::Widget;

    void paint_callback() override;

    void setProcessorStatus(st_dspStatus *status);

    void setTaskStatus(st_dspStatus *status);

protected:

    void do_paint() override;

    st_dspStatus *processor_status;
    st_dspStatus *task_status;
};

#endif //TRX_FRONTEND_SIGNAL_GENERATOR_WIDGET_H
