//
// Created by Angel Dust on 17/04/2021.
//

#include "capture_widget.h"
#include "Display_afb.h"

void CaptureWidget::paint_callback() {

    char buff[30];
    this->display->setBgColor(C565_DARKEST);
    display->clear();
    display->setFont((FontDef *)&Font_Tiny8x8);
    display->gotoCharXY(0, 0);
    display->setColor(C565_WHITE);
    display->setBgColor(C565_TRANSPARENT);

    if (task_status) {
        float seconds_elapsed = 0;

        seconds_elapsed = task_status->elapsed_ms() / 1000.0;
        float drop_rate = processor_status->drop_rate() * 100;
        float bytes_processed = (processor_status->processed_blocks) * processor_status->block_size_bytes;
        float bytes_stored = (task_status->processed_blocks) * DSP_FIFO_BLOCK_BYTES;

        uint16_t c = C565_WHITE;

        switch (task_status->status) {
            case DSP_STATUS_RUNNING:
                c = C565_BLUE;
                sprintf(buff, "Running\n");
                break;
            case DSP_STATUS_STOPPED:
                if (task_status->error == DSP_ERR_NONE) {
                    if (task_status->stop_ms) {
                        c = C565_GREEN;
                        sprintf(buff, "Finished\n");
                    } else {
                        c = C565_GREY_LIGHT;
                        sprintf(buff, "Stopped\n");
                    }
                } else {

                    c = C565_RED;
                    sprintf(buff, "Error\n");
                    break;
                }
                break;
            case DSP_STATUS_PENDING:
                c = C565_WHITE;
                sprintf(buff, "Pending\n");
                break;
        }

        display->print("Status: ");
        display->setColor(c);
        display->print(buff);

        if (task_status->error != DSP_ERR_NONE) {
            display->setColor(C565_RED);
            display->print(dsp::dsp_error_names[task_status->error]);
            display->print("\n");
        }

        display->setColor(C565_WHITE);
        sprintf(buff, "%.1f", seconds_elapsed);
        display->print("Elapsed:", buff, " s.\n");
        char new_units[5];
        format_eng(buff, bytes_processed, "b.\n", new_units);
        display->print("In:", buff, new_units);
        format_eng(buff, bytes_stored, "b.\n", new_units);
        display->print("Out:", buff, new_units);

        sprintf(buff, "%.1f", drop_rate);
        display->print("Drop:", buff, "%\n");
    } else {
        display->print("NO STATUS");
    }
}

void CaptureWidget::before_paint() {
    uint64_t m = HAL_GetTick();
    if (m - last_refresh_ms > 100 || dirty()) {

        set_dirty();
    }
}

void CaptureWidget::setTaskStatus(st_dsp_status *status) {
    CaptureWidget::task_status = status;
}

void CaptureWidget::setProcessorStatus(st_dsp_status *status) {
    CaptureWidget::processor_status = status;
}
