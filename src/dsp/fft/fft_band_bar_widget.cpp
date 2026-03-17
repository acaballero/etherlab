//
// Created by Angel Dust on 14/03/2026.
//

#include "fft_band_bar_widget.h"

#include "dsp/fft/fft_params.h"
#include "fft.h"
#include "ips_font.h"
#include "os/task_manager.h"
#include "radio.h"
#include "ui/frequency_memory_ui.h"

#include <algorithm>
#include <cstddef>
#include <cstring>

static constexpr uint32_t MAX_BAND_MARKERS = 64;

static uint16_t band_color_for_index(size_t ix) {
    // Use darker tones so the band bar doesn't dominate the UI.
    static const uint16_t colors[] = {
        C565_CYAN_DARK, C565_GREEN_DARK, C565_NAVY, C565_PURPLE, C565_MAROON, C565_GREY_DARKER, C565_OLIVE,
    };

    return colors[ix % (sizeof(colors) / sizeof(colors[0]))];
}

static void fit_ellipsis_label(const char *src, int available_px, char *dst, size_t dst_size, int char_width_px) {
    if (!src || !dst || dst_size == 0) {
        return;
    }

    dst[0] = '\0';

    if (available_px <= 0 || char_width_px <= 0) {
        return;
    }

    size_t len = strlen(src);
    int max_chars = available_px / char_width_px;

    if (max_chars <= 0) {
        return;
    }

    if ((int)len <= max_chars) {
        strncpy(dst, src, dst_size - 1);
        dst[dst_size - 1] = '\0';
        return;
    }

    // Need at least one character + "..."
    if (max_chars < 4) {
        return;
    }

    int keep = max_chars - 3;
    if (keep <= 0) {
        return;
    }

    size_t copy_n = (size_t)keep;
    if (copy_n > dst_size - 4) {
        copy_n = dst_size - 4;
    }

    memcpy(dst, src, copy_n);
    memcpy(dst + copy_n, "...", 3);
    dst[copy_n + 3] = '\0';
}

FFTBandBarWidget::FFTBandBarWidget(const Rect &parentRect, Display *display) : Widget(parentRect, display) {

    radio::freq_signal.add(this, [this](void *, const void *) {
        static int task_id;

        if (visible()) {
            if (task_id != -1) {
                os::task_manager.remove(task_id);
            }

            task_id = os::task_manager.set_timeout(500, [this]() {
                refresh_all = true;
                set_dirty();
            });
        }
    });

    fft::signal.add(this, [this](void *, const void *) {
        refresh_all = true;
        set_dirty();
    });
}

bool FFTBandBarWidget::on_touch(const st_inputEvent) {
    return false;
}

void FFTBandBarWidget::fetch_bands_in_range() {

    uint64_t span_start = fft::fft_params.span_f_start;
    uint64_t span_end = span_start + fft::fft_params.span;

    band_markers.clear();

    // Markers fully inside the span
    freq_memory::find_in_freq_range(span_start, span_end, band_markers, {BAND_START, BAND_END}, MAX_BAND_MARKERS);

    // Include the band start before the window, so bands crossing span_start can be drawn.
    st_freq_mem prev_start = freq_memory::find_closest(span_start, BACKWARDS, BAND_START);
    if (prev_start.freq) {
        band_markers.push_back(prev_start);
    }

    // Ensure the band end for the last band start <= span_end is present so a band crossing span_end can be drawn.
    st_freq_mem last_start = freq_memory::find_closest(span_end, BACKWARDS, BAND_START);
    if (last_start.freq) {
        band_markers.push_back(last_start);

        st_freq_mem end_for_last = freq_memory::find_closest(last_start.freq, FORWARD, BAND_END);
        if (end_for_last.freq) {
            band_markers.push_back(end_for_last);
        }
    }

    std::sort(band_markers.begin(), band_markers.end(), [](const st_freq_mem &a, const st_freq_mem &b) {
        if (a.freq != b.freq) {
            return a.freq < b.freq;
        }
        return a.id < b.id;
    });

    // Dedup by id (st_freq_mem equality is id-based)
    band_markers.erase(std::unique(band_markers.begin(), band_markers.end(),
                                   [](const st_freq_mem &a, const st_freq_mem &b) {
                                       return a.id == b.id;
                                   }),
                       band_markers.end());
}

void FFTBandBarWidget::build_segments() {

    segments.clear();
    db_error = false;

    if (band_markers.empty()) {
        return;
    }

    uint64_t span_start = fft::fft_params.span_f_start;
    uint64_t span_end = span_start + fft::fft_params.span;

    bool have_start = false;
    st_freq_mem start_marker{};

    for (const auto &m : band_markers) {

        if (m.type == BAND_START) {
            start_marker = m;
            have_start = true;
            continue;
        }

        if (m.type == BAND_END) {
            if (!have_start) {
                db_error = true;
                continue;
            }

            if (strncmp(start_marker.name, m.name, FREQ_MEM_NAME_SIZE) != 0) {
                // Adjacent START/END must match; otherwise DB is inconsistent.
                db_error = true;
                have_start = false;
                continue;
            }

            band_segment seg;
            seg.start_hz = start_marker.freq;
            seg.end_hz = m.freq;
            strncpy(seg.name, start_marker.name, FREQ_MEM_NAME_SIZE);
            seg.name[FREQ_MEM_NAME_SIZE] = '\0';

            // Keep only segments intersecting the visible span
            if (seg.end_hz > span_start && seg.start_hz < span_end) {
                segments.push_back(seg);
            }

            have_start = false;
        }
    }

    if (have_start) {
        // Unmatched start marker.
        db_error = true;
    }
}

bool FFTBandBarWidget::paint_callback() {

    display->clear();

    if (refresh_all) {
        fetch_bands_in_range();
        build_segments();
        refresh_all = false;
    }

    if (segments.empty() || fft::fft_params.span == 0) {
        return true;
    }

    const uint16_t w = size().width();
    const uint16_t h = size().height();

    const uint16_t y_label = 2;

    const uint64_t span_start = fft::fft_params.span_f_start;
    const uint64_t span = fft::fft_params.span;
    const uint64_t span_end = span_start + span;

    auto *font = (FontDef *)&Font_Fixed5x7;
    display->setFont(font);
    display->setBgColor(C565_TRANSPARENT);

    for (size_t i = 0; i < segments.size(); i++) {
        const auto &seg = segments[i];

        uint64_t s = seg.start_hz < span_start ? span_start : seg.start_hz;
        uint64_t e = seg.end_hz > span_end ? span_end : seg.end_hz;
        if (e <= s) {
            continue;
        }

        int x1 = (int)(((float)(s - span_start) / (float)span) * (float)w);
        int x2 = (int)(((float)(e - span_start) / (float)span) * (float)w);

        x1 = constrain(x1, 0, (int)w - 1);
        x2 = constrain(x2, 0, (int)w - 1);
        if (x2 <= x1) {
            x2 = x1 + 1;
        }

        const uint16_t seg_color = band_color_for_index(i);

        // Segment separators
        display->writeVertLine(x1, 1, h - 2, C565_GREY_DARKER);
        display->writeVertLine(x2, 1, h - 2, C565_GREY_DARKER);

        // Slanted hatch (45 degrees). Using setPixel here is OK because the bar is small.
        const int y0 = 1;
        const int y1 = (int)h - 2;
        const int period = 6;

        for (int y = y0; y <= y1; y++) {
            // Clip horizontally to avoid overwriting separators.
            for (int x = x1 + 1; x <= x2 - 1; x++) {
                // 45-degree stripes
                if ((((x - x1) + (y - y0)) % period) == 0) {
                    display->setPixel(x, y, seg_color);
                }
            }
        }

        // Subtle borders
        display->writeLine(x1 + 1, y0, x2 - 1, y0, C565_GREY_DARKER);
        display->writeLine(x1 + 1, y1, x2 - 1, y1, C565_GREY_DARKER);

        // Label (ellipsis if needed, omit if still not possible). Draw black background for readability.
        const int seg_w = (x2 - x1);
        const int avail_px = seg_w - 6;

        char label[FREQ_MEM_NAME_SIZE + 1];
        fit_ellipsis_label(seg.name, avail_px, label, sizeof(label), font->width);

        if (label[0]) {
            Size ts = display->get_text_size(label);
            if ((int)ts.width() <= avail_px && y_label + ts.height() < h) {
                int tx = x1 + (seg_w - (int)ts.width()) / 2;
                int bx0 = tx - 2;
                int bx1 = tx + (int)ts.width() + 2;
                int by0 = y_label - 1;
                int by1 = y_label + (int)ts.height() - 2;

                bx0 = max2(bx0, x1 + 1);
                bx1 = min2(bx1, x2 - 1);

                display->fill(bx0, by0, bx1, by1, C565_BLACK);
                display->setBgColor(C565_BLACK);
                display->setColor(C565_GREY_LIGHT);
                display->gotoXY(tx, y_label);
                display->print(label);
            }
        }
    }

    if (db_error) {
        display->setColor(C565_RED);
        display->gotoXY(2, 2);
        display->print("DB!");
    }

    return true;
}

void FFTBandBarWidget::before_paint() {
}
