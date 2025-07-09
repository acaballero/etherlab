//
// Created by Angel Dust on 17/04/2021.
//

#include "replay_widget.h"
#include "../../../lib/utils/utils.hpp"
#include "Display_afb.h"
#include "ips_font.h"

void ReplayWidget::paint_callback() {

    char buff[20];
    float seconds_elapsed = 0;

    display->fillBuffer(C565_DARKEST);

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

    display->setFont((FontDef *)&Font_7x10);
    // display->setFont((FontDef *)&Font_Tiny8x8);
    display->setVerticalLineSpacing(1);
    display->setPadding(10, 10);

    display->gotoCharXY(0, 0);
    display->setColor(C565_WHITE);
    display->setBgColor(C565_TRANSPARENT);

    display->print("Status: ");
    display->setColor(c);
    display->print(buff);

    display->gotoCharXY(0, 1);

    switch (wi.format) {

        case FSTATUS_NONE:
            display->setColor(C565_YELLOW);
            display->print("Select a file\n");
            break;
        case FSTATUS_ERROR:
        case FSTATUS_INVALID:
        default:
            display->setColor(C565_RED);
            display->print("Invalid format\n");
            break;
        case FSTATUS_OK:
            char units[5];

            if (wi.sample_rate) {
                format_eng(buff, wi.sample_rate, "Hz\n", units, 3, true);
                display->print("Rate: ", buff, units);
            }

            if (wi.carrier_freq) {
                format_eng(buff, wi.carrier_freq, "Hz\n", units, 3, true);
                display->print("Freq: ", buff, units);
            }

            format_eng(buff, finfo.fsize, "b\n", units);
            display->print("Size: ", buff, units);

            break;
    }

    if (task_status->status == DSP_STATUS_RUNNING || task_status->stop_ms) { // If it's running or just finished

        if (task_status->status == DSP_STATUS_RUNNING) {
            seconds_elapsed = (HAL_GetTick() - task_status->start_ms) / 1000.0;
        } else {
            seconds_elapsed = (task_status->stop_ms - task_status->start_ms) / 1000.0;
        }
        float drop_rate =
            processor_status->processed_blocks ? ((float)processor_status->fifo_underruns / (float)processor_status->processed_blocks) * 100.0 : 0;
        float bytes_processed = (processor_status->processed_blocks - processor_status->fifo_underruns) * processor_status->block_size_bytes;

        float bytes_decimated = (processor_status->processed_blocks - processor_status->fifo_underruns) * processor_status->decimated_block_size_bytes;

        char new_units[5];
        display->setColor(C565_WHITE);
        sprintf(buff, "%.1f", seconds_elapsed);
        display->print("Elapsed: ", buff, " s.\n");
        format_eng(buff, bytes_decimated, "b.\n", new_units);
        display->print("In: ", buff, new_units);
        format_eng(buff, bytes_processed, "b.\n", new_units);
        display->print("Out: ", buff, new_units);
        sprintf(buff, "%.1f", drop_rate);
        display->print("Drop: ", buff, "%\n");
    }
}

void ReplayWidget::before_paint() {
    uint64_t m = HAL_GetTick();
    if (m - last_refresh_ms > 100 || dirty()) {
        set_dirty();
    }
}

void ReplayWidget::setTaskStatus(st_dsp_status *status) {
    ReplayWidget::task_status = status;
}

void ReplayWidget::setProcessorStatus(st_dsp_status *status) {
    ReplayWidget::processor_status = status;
}

void ReplayWidget::setWaveInfo(WaveInfo wi) {
    ReplayWidget::wi = wi;
    set_dirty();
}

void ReplayWidget::setFileInfo(FILINFO finfo) {
    ReplayWidget::finfo = finfo;
    set_dirty();
}
