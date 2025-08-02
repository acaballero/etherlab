//
// Created by Angel Dust on 18/05/2025.
//
#include "aprs_table_widget.h"
#include "Display_afb.h"
#include "dsp/aprs/aprs_packet.h"
#include "dsp/aprs/aprs_rx_task.h"
#include "hw/stm32f4xx/rtc.h"
#include "input/inputEvent.h"
#include "ips_font.h"
#include "status.h"
#include <cstddef>
#include <cstring>
#include <iterator>

namespace dsp_ui {

bool APRSTableWidget::paint_callback() {

    char buf[26];

    display->clear();

    display->setFont(this->font);
    display->set_trim_enabled(false);
    display->setColor(C565_GREY_DARKER);
    display->setVerticalLineSpacing(line_spacing);
    display->drawRoundedRectangle(0, 0, area.box.width, title_height, 2, true);
    display->gotoXY(4, 3);
    sprintf(buf, "%-7s %5s %-8s\n", "Source", "Hits", "Time");
    display->setBgColor(C565_GREY_DARKER);
    display->setColor(C565_WHITE);
    display->print(buf);

    display->setBgColor(C565_BLACK);

    for (size_t i = 0; i < sources.size(); i++) {
        APRSSource *source = &sources[i];
        int y = title_height + 3 + (i * line_height);

        display->gotoXY(4, y);

        display->setColor(palette16[source->id]);
        display->setBgColor(C565_TRANSPARENT);

        if (source->id == selected_id) {
            display->drawRoundedRectangle(0, y - 2, area.box.width, line_height, 2, false);
        }

        snprintf(buf, sizeof(buf), "%-7s%s %s%3d %-8s\n", source->source_formatted, source->has_position ? "*" : "", source->hits <= 999 ? " " : "+",
                 source->hits <= 999 ? source->hits : 999, source->time_string);

        display->print(buf);
    }

    display->set_trim_enabled(true);

    return true;
}

void APRSTableWidget::before_paint(){};

bool APRSTableWidget::on_touch(const st_inputEvent e) {

    size_t ix = ((e.point - screen_pos()).y() - title_height) / line_height;
    if (ix >= 0 && ix < sources.size()) {
        select(ix);
    }

    return true;
}

void APRSTableWidget::select(int ix) {

    selected_id = sources[ix].id;
    set_dirty();
}

bool APRSTableWidget::on_input(const st_inputEvent e) {

    bool consumed = Widget::on_input(e);

    if (consumed) {
        return true;
    }

    size_t i = 0;
    switch (e.type) {

        case INPUT_EVENT_TYPE_ENCODER:

            for (i = 0; i < sources.size(); i++) {
                if (selected_id == sources[i].id) {
                    break;
                }
            }

            if (i == sources.size()) {
                i = 0;
            } else {
                i += e.value > 0 ? 1 : -1;
            }

            if (i >= 0 && i < sources.size() && sources[i].id != selected_id) {
                select(i);
            }

            consumed = true;
            break;

        case INPUT_EVENT_TYPE_BUTTON_PRESS:
            switch (e.value) {
                case BTN_ENCODER:

                    for (i = 0; i < sources.size(); i++) {
                        if (selected_id == sources[i].id) {
                            on_select(sources[i]);

                            break;
                        }
                    }
                    consumed = true;

                    break;
                default:
                    consumed = false;
                    break;
            }
            break;

        default:
            consumed = false;
            break;
    }

    return consumed;
}

void APRSTableWidget::init() {
    line_height = font->height + line_spacing;
    title_height = this->font->height + 5;
}

int APRSTableWidget::find_free_id() {

    bool used[max_sources];
    int id = -1;

    for (int i = 0; i < max_sources; i++) {
        used[i] = false;
    }

    for (size_t i = 0; i < sources.size(); i++) {
        APRSSource s = sources[i];
        if (s.id >= 0) {
            used[s.id] = true;
        }
    }

    for (size_t i = 0; i < sources.size(); i++) {
        APRSSource s = sources[i];
        if (s.id == -1) {
            for (int i = 0; i < max_sources; ++i) {
                if (!used[i]) {
                    id = i;
                    break;
                }
            }
            break;
        }
    }

    return id;
}

int APRSTableWidget::on_packet(dsp::APRSPacket *packet) {

    APRSSource *entry = sources.find([&](const APRSSource &s) {
        return s.source == packet->get_source();
    });

    bool isnew = false;

    if (!entry) {
        entry = sources.push({});
        entry->source = packet->get_source();
        isnew = true;
    }

    st_datetime dt = rtc_get_date_time();
    uint32_t ts = rtc_to_epoch(&dt.time, &dt.date);
    entry->age = ts;
    entry->hits++;

    char buff[20];

    rtc_to_string(dt, true, buff);
    snprintf(entry->time_string, APRSSource::time_length + 1, "%s", buff);

    packet->get_source_formatted(buff);
    snprintf(entry->source_formatted, APRSSource::source_length + 1, "%s", buff);

    if (entry->has_position && !packet->has_position()) {
        // Just to clarify that we maintain last position info of this source, if it has one from a previous packet
    } else {
        entry->has_position = packet->has_position();
        entry->pos = packet->get_position();
    }

    int id = entry->id;

    // Sort by age
    sources.sort([&](const APRSSource &a, const APRSSource &b) {
        return a.age > b.age;
    });

    if (sources.size() > max_sources) {
        // sources.clear();
        sources.erase_last(sources.size() - max_sources);
    }

    // Find it again (not really needed at the moment since we are assigning now() as its timestamp and so it will be the first)

    entry = sources.find([id](const APRSSource &i) {
        return i.id == id;
    });

    if (isnew) {
        entry->id = find_free_id();
    }

    set_dirty();

    return entry->id;
}

} // namespace dsp_ui
