//
// OOK Widget - Draws OOK sequence timing diagram with proportional mark/space
//

#include "ook_widget.h"
#include "Display_afb.h"
#include "dsp/dsp.h"
#include "dsp/ook/dsp_ook_processor.h"
#include "ips_font.h"
#include "utils.hpp"
#include <cstdio>

void OOKWidget::draw_waveform() {

    if (!sequence || sequence->empty()) {
        display->setFont((FontDef *)&Font_7x10);
        display->setColor(C565_YELLOW);
        display->gotoCharXY(0, 2);
        display->print("No sequence defined");
        return;
    }

    const int margin_x = 6;
    const int y_top = 16;

    const int bar_y = area.box.height - 8;
    const int label_y = bar_y - y_top; // Font_7x10 + spacing
    const int y_bottom = label_y - 6;  // Leave room for labels

    const int diagram_width = area.box.width - margin_x * 2;

    if (y_bottom <= y_top) {
        return;
    }

    size_t seq_len = sequence->size();

    // Compute total "time" of the sequence in microseconds for proportional drawing
    uint32_t total_us = 0;
    for (size_t i = 0; i < seq_len; i++) {
        total_us += ((*sequence)[i] != 0) ? mark_duration_us : space_duration_us;
    }

    if (total_us == 0) {
        return;
    }

    float px_per_us = (float)diagram_width / (float)total_us;

    uint16_t line_color = C565_CYAN;
    uint16_t cursor_color = C565_YELLOW;

    float x_accum = (float)margin_x;
    int prev_y = y_bottom; // Start low (space)

    // Cursor X position (accumulated up to current_bit)
    float cursor_x = -1;

    for (size_t i = 0; i < seq_len; i++) {
        bool on = ((*sequence)[i] != 0);
        uint32_t bit_us = on ? mark_duration_us : space_duration_us;
        float bit_px = (float)bit_us * px_per_us;

        int x_start = (int)(x_accum + 0.5f);
        int x_end = (int)(x_accum + bit_px + 0.5f);

        // Clamp
        if (x_end > margin_x + diagram_width) {
            x_end = margin_x + diagram_width;
        }
        if (x_start >= x_end) {
            x_accum += bit_px;
            continue;
        }

        int y = on ? y_top : y_bottom;

        // Vertical transition
        if (y != prev_y && x_start > margin_x) {
            display->writeLine(x_start, prev_y, x_start, y, line_color);
        }

        // Horizontal line at current level
        if (x_end > x_start) {
            display->writeLine(x_start, y, x_end, y, line_color);
        }

        // Track cursor position
        if (i == current_bit) {
            cursor_x = x_accum;
        }

        prev_y = y;
        x_accum += bit_px;
    }

    // Close the waveform back to low at the end
    if (prev_y != y_bottom) {
        int x_end = (int)(x_accum + 0.5f);
        if (x_end > margin_x + diagram_width) {
            x_end = margin_x + diagram_width;
        }
        display->writeLine(x_end, prev_y, x_end, y_bottom, line_color);
    }

    // Baseline
    display->writeLine(margin_x, y_bottom, margin_x + diagram_width, y_bottom, C565_GREY_DARK);

    // Labels
    display->setFont((FontDef *)&Font_7x10);
    display->setColor(C565_GREY_LIGHT);
    display->gotoXY(margin_x, label_y);

    char label[20];
    snprintf(label, sizeof(label), "%d bits", (int)seq_len);
    display->print(label);

    // Frame duration label (right-aligned)
    snprintf(label, sizeof(label), "%lu us", (unsigned long)total_us);
    int label_w = strlen(label) * 7;
    display->gotoXY(margin_x + diagram_width - label_w, label_y);
    display->print(label);

    // Draw playback cursor
    if (task_status && task_status->status == DSP_STATUS_RUNNING && cursor_x >= 0) {
        int cx = (int)(cursor_x + 0.5f);
        display->writeLine(cx, y_top - 2, cx, y_bottom + 1, cursor_color);
    }
}

void OOKWidget::draw_progress_bar() {

    if (!task_status || task_status->status != DSP_STATUS_RUNNING || total_reps <= 1) {
        return;
    }

    const int margin_x = 6;
    const int bar_y = area.box.height - 8;
    const int bar_h = 4;
    const int bar_w = area.box.width - margin_x * 2;

    // Background
    for (int y = bar_y; y < bar_y + bar_h; y++) {
        display->writeLine(margin_x, y, margin_x + bar_w, y, C565_GREY_DARK);
    }

    // Fill
    if (total_reps > 0) {
        int fill_w = (int)((float)(current_rep) / (float)total_reps * bar_w);
        if (fill_w > bar_w)
            fill_w = bar_w;
        for (int y = bar_y; y < bar_y + bar_h; y++) {
            display->writeLine(margin_x, y, margin_x + fill_w, y, C565_GREEN);
        }
    }

    // Rep counter label (above bar)
    display->setFont((FontDef *)&Font_Fixed5x7);
    display->setColor(C565_GREY_LIGHT);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d/%d", current_rep + 1, total_reps);
    int lw = strlen(buf) * 5;
    display->gotoXY(margin_x + bar_w - lw, bar_y - 8);
    display->print(buf);
}

bool OOKWidget::paint_callback() {

    display->fillBuffer(C565_BLACK);
    display->writeRect({0, 0}, {2, area.box.height}, C565_DARKEST);

    display->setFont((FontDef *)&Font_7x10);
    display->setVerticalLineSpacing(1);
    display->set_padding(4, 4);
    display->setBgColor(C565_TRANSPARENT);
    display->gotoCharXY(0, 0);

    // Status line
    if (task_status) {
        const char *status_text = "Stopped";
        uint16_t status_color = C565_GREY_LIGHT;

        switch (task_status->status) {
            case DSP_STATUS_RUNNING:
                status_text = "Transmitting";
                status_color = C565_GREEN;
                break;
            case DSP_STATUS_PENDING:
                status_text = "Pending";
                status_color = C565_WHITE;
                break;
            case DSP_STATUS_STOPPED:
                if (task_status->error != DSP_ERR_NONE) {
                    status_text = "Error";
                    status_color = C565_RED;
                } else if (task_status->stop_ms) {
                    status_text = "Finished";
                    status_color = C565_BLUE;
                }
                break;
            case DSP_STATUS_STOPPING:
                status_text = "Stopping";
                status_color = C565_YELLOW;
                break;
        }

        display->setColor(status_color);
        display->print(status_text);
    }

    draw_waveform();
    draw_progress_bar();

    return true;
}

void OOKWidget::before_paint() {
    uint64_t m = HAL_GetTick();

    // Pull live state from processor during TX
    if (task_status && task_status->status == DSP_STATUS_RUNNING) {
        if (dsp_task) {
            auto *proc = (DspOOKProcessor *)dsp_task->get_processor();
            if (proc) {
                current_bit = proc->get_current_bit_index();
                current_rep = proc->get_current_rep();
                total_reps = proc->get_repetitions();
            }
        }
    }

    if (m - this->last_refresh_ms > 80 || this->dirty()) {
        this->set_dirty();
    }
}
