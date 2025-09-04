//
// Created by Angel Dust on 17/04/2021.
//

#include "replay_widget.h"
#include "../../../lib/utils/utils.hpp"
#include "Display_afb.h"
#include "ips_font.h"

bool ReplayWidget::paint_callback() {

    char buff[40];

    display->fillBuffer(C565_DARKEST);

    uint16_t c = C565_WHITE;

    int elapsed_s = task_status->elapsed_ms() * 1000;

    switch (task_status->status) {
        case DSP_STATUS_RUNNING:
            c = C565_BLUE;
            sprintf(buff, "Running (%.1fs)\n", elapsed_s);
            break;
        case DSP_STATUS_STOPPED:
            if (task_status->error == DSP_ERR_NONE) {
                if (task_status->stop_ms) {
                    c = C565_GREEN;
                    sprintf(buff, "Finished (%.1fs)\n", elapsed_s);
                } else {
                    c = C565_GREY_LIGHT;
                    sprintf(buff, "Stopped\n");
                }
            } else {
                c = C565_RED;
                sprintf(buff, "Error (%.1fs)\n", elapsed_s);
                break;
            }
            break;
        case DSP_STATUS_PENDING:
            c = C565_WHITE;
            sprintf(buff, "Pending (%.1fs)\n", elapsed_s);
            break;
        case DSP_STATUS_STOPPING:
            break;
    }

    display->setFont((FontDef *)&Font_7x10);
    // display->setFont((FontDef *)&Font_Tiny8x8);
    display->setVerticalLineSpacing(1);
    display->set_padding(10, 10);

    display->gotoCharXY(0, 0);
    display->setColor(C565_WHITE);
    display->setBgColor(C565_TRANSPARENT);

    display->print("Status: ");
    display->setColor(c);
    display->print(buff);
    display->print("\n");

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

            format_eng(buff, wi.file_size, "b\n", units);
            display->print("Size: ", buff, units);

            break;
    }

    if (wi.format == FSTATUS_OK && (task_status->status == DSP_STATUS_RUNNING || task_status->stop_ms)) { // If it's running or just finished

        float bytes_processed = (processor_status->processed_blocks - processor_status->fifo_underruns) * processor_status->block_size_bytes;
        float bytes_decimated = (processor_status->processed_blocks - processor_status->fifo_underruns) * processor_status->decimated_block_size_bytes;

        char u1[5], u2[5];
        char v1[10], v2[10];
        display->setColor(C565_WHITE);

        format_eng(v1, bytes_decimated, "b.", u1);
        format_eng(v2, bytes_processed, "b.", u2);
        sprintf(buff, "In/out: %s %s / %s %s\n", v1, u1, v2, u2);
        display->print(buff);
        sprintf(buff, "%.1f", processor_status->drop_rate() * 100);
        display->print("Drop: ", buff, "%\n");
    }

    return true;
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
