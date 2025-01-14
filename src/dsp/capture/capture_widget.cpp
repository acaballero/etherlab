//
// Created by Angel Dust on 17/04/2021.
//

#include "capture_widget.h"

void CaptureWidget::paint_callback() {

    char buff[30];
    this->display->clear();
    this->display->setFont((FontDef *)&Font_Tiny8x8);
    this->display->gotoCharXY(0, 0);
    this->display->setColor(C565_WHITE);
    this->display->setBgColor(C565_TRANSPARENT);

    if (this->task_status) {
        float seconds_elapsed = 0;

        if (this->task_status->status == DSP_STATUS_RUNNING) {
            seconds_elapsed = (HAL_GetTick() - this->task_status->start_ms) / 1000.0;
        } else {
            seconds_elapsed = (this->task_status->stop_ms - this->task_status->start_ms) / 1000.0;
        }

        float drop_rate = this->processor_status->processed_blocks
                              ? (((float)this->processor_status->fifo_overruns / (float)this->processor_status->processed_blocks) * 100.0)
                              : 0;
        float bytes_processed = (this->processor_status->processed_blocks) * this->processor_status->block_size_bytes;

        float bytes_stored = (this->task_status->processed_blocks) * DSP_FIFO_BLOCK_BYTES;

        uint16_t c = C565_WHITE;

        switch (this->task_status->status) {
            case DSP_STATUS_RUNNING:
                c = C565_BLUE;
                sprintf(buff, "Running\n");
                break;
            case DSP_STATUS_STOPPED:
                if (this->task_status->error == DSP_ERR_NONE) {
                    if (this->task_status->stop_ms) {
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

        this->display->print("Status: ");
        this->display->setColor(c);
        this->display->print(buff);

        if (this->task_status->error != DSP_ERR_NONE) {
            this->display->setColor(C565_RED);
            this->display->print(dsp_error_names[this->task_status->error]);
            this->display->print("\n");
        }

        this->display->setColor(C565_WHITE);
        sprintf(buff, "%.1f", seconds_elapsed);
        this->display->print("Elapsed:", buff, " s.\n");
        char new_units[5];
        format_eng(buff, bytes_processed, "b.\n", new_units);
        this->display->print("In:", buff, new_units);
        format_eng(buff, bytes_stored, "b.\n", new_units);
        this->display->print("Out:", buff, new_units);

        sprintf(buff, "%.1f", drop_rate);
        this->display->print("Drop:", buff, "%\n");
    } else {
        this->display->print("NO STATUS");
    }
}

void CaptureWidget::before_paint() {
    uint64_t m = HAL_GetTick();
    if (m - this->last_refresh_ms > 100 || this->dirty()) {

        this->set_dirty();
    }
}

void CaptureWidget::setTaskStatus(st_dspStatus *status) { CaptureWidget::task_status = status; }

void CaptureWidget::setProcessorStatus(st_dspStatus *status) { CaptureWidget::processor_status = status; }
