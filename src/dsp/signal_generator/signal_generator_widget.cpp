//
// Created by Angel Dust on 17/04/2021.
//

#include "signal_generator_widget.h"
#include "../../../lib/utils/utils.hpp"
#include "dsp/dsp_common.h"

bool SignalGeneratorWidget::paint_callback() {

    char buff[20];

    this->display->clear();
    //    uint16_t c = C565_WHITE;

    switch (this->task_status->status) {
        case DSP_STATUS_RUNNING:
            //  c = C565_BLUE;
            sprintf(buff, "Running\n");
            break;
        case DSP_STATUS_STOPPED:
            if (this->task_status->error == DSP_ERR_NONE) {
                if (this->task_status->stop_ms) {
                    //   c = C565_GREEN;
                    sprintf(buff, "Finished\n");
                } else {
                    //   c = C565_GREY_LIGHT;
                    sprintf(buff, "Stopped\n");
                }
            } else {
                //   c = C565_RED;
                sprintf(buff, "Error\n");
                break;
            }
            break;
        case DSP_STATUS_PENDING:
            //   c = C565_WHITE;
            sprintf(buff, "Pending\n");
            break;
        case DSP_STATUS_STOPPING:
            sprintf(buff, "Stopping\n");
            break;
    }

    return true;
}

void SignalGeneratorWidget::before_paint() {
    uint64_t m = HAL_GetTick();
    if (m - this->last_refresh_ms > 100 || this->dirty()) {

        this->set_dirty();
    }
}

void SignalGeneratorWidget::setTaskStatus(st_dsp_params *status) {
    SignalGeneratorWidget::task_status = status;
}

void SignalGeneratorWidget::setProcessorStatus(st_dsp_params *status) {
    SignalGeneratorWidget::processor_status = status;
}
