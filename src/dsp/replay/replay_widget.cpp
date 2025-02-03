//
// Created by Angel Dust on 17/04/2021.
//

#include "replay_widget.h"
#include "../../../lib/utils/utils.hpp"

void ReplayWidget::paint_callback() {

    char buff[20];
    float seconds_elapsed = 0;

    this->display->clear();

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

    // this->display->setFont((FontDef *)&Font_Fixed5x7);
    this->display->setFont((FontDef *)&Font_Tiny8x8);
    this->display->setVerticalLineSpacing(1);

    this->display->gotoCharXY(0, 0);
    this->display->setColor(C565_WHITE);
    this->display->setBgColor(C565_TRANSPARENT);

    this->display->print("Status: ");
    this->display->setColor(c);
    this->display->print(buff);

    this->display->gotoCharXY(0, 1);

    switch (this->wi.format) {

        case FSTATUS_NONE:
            this->display->setColor(C565_YELLOW);
            this->display->print("Select a file\n");
            break;
        case FSTATUS_ERROR:
        case FSTATUS_INVALID:
        default:
            this->display->setColor(C565_RED);
            this->display->print("Invalid format\n");
            break;
        case FSTATUS_OK:
            char units[5];

            if (this->wi.sample_rate) {
                format_eng(buff, this->wi.sample_rate, "Hz\n", units, 3, true);
                this->display->print("Rate: ", buff, units);
            }

            if (this->wi.carrier_freq) {
                format_eng(buff, this->wi.carrier_freq, "Hz\n", units, 3, true);
                this->display->print("Freq: ", buff, units);
            }

            format_eng(buff, this->finfo.fsize, "b\n", units);
            this->display->print("Size: ", buff, units);

            break;
    }

    if (this->task_status->status == DSP_STATUS_RUNNING || this->task_status->stop_ms) { // If it's running or just finished

        if (this->task_status->status == DSP_STATUS_RUNNING) {
            seconds_elapsed = (HAL_GetTick() - this->task_status->start_ms) / 1000.0;
        } else {
            seconds_elapsed = (this->task_status->stop_ms - this->task_status->start_ms) / 1000.0;
        }
        float drop_rate = this->processor_status->processed_blocks
                              ? ((float)this->processor_status->fifo_underruns / (float)this->processor_status->processed_blocks) * 100.0
                              : 0;
        float bytes_processed = (this->processor_status->processed_blocks - this->processor_status->fifo_underruns) * this->processor_status->block_size_bytes;

        float bytes_read =
            (this->processor_status->processed_blocks - this->processor_status->fifo_underruns) * this->processor_status->decimated_block_size_bytes;

        char new_units[5];
        this->display->setColor(C565_WHITE);
        sprintf(buff, "%.1f", seconds_elapsed);
        this->display->print("Elapsed: ", buff, " s.\n");
        format_eng(buff, bytes_processed, "b.\n", new_units);
        this->display->print("In: ", buff, new_units);
        format_eng(buff, bytes_read, "b.\n", new_units);
        this->display->print("Out: ", buff, new_units);
        sprintf(buff, "%.1f", drop_rate);
        this->display->print("Miss: ", buff, "%\n");
    }

    if (this->wi.format != FSTATUS_NONE && show_actions) {
        this->display->setFont((FontDef *)&Font_Fixed5x7);
        this->display->gotoCharXY(0, 6);
        this->display->print("Long press to delete");
    }
}

void ReplayWidget::before_paint() {
    uint64_t m = HAL_GetTick();
    if (m - this->last_refresh_ms > 100 || this->dirty()) {
        this->set_dirty();
    }
}

void ReplayWidget::setTaskStatus(st_dspStatus *status) { ReplayWidget::task_status = status; }

void ReplayWidget::setProcessorStatus(st_dspStatus *status) { ReplayWidget::processor_status = status; }

void ReplayWidget::setWaveInfo(WaveInfo wi) {
    ReplayWidget::wi = wi;
    this->set_dirty();
}

void ReplayWidget::setFileInfo(FILINFO finfo) {
    ReplayWidget::finfo = finfo;
    this->set_dirty();
}
