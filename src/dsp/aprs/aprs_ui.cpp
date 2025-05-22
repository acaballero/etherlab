//
// Created by Angel Dust on 18/05/2025.
//
#include "aprs_ui.h"
#include "Display_afb.h"
#include "dsp/aprs/aprs_packet.h"
#include "dsp/aprs/aprs_task.hpp"
#include "dsp/dsp_tasks.h"
#include "hw/stm32f4xx/rtc.h"
#include "ips_font.h"
#include "main_board.h"
#include "types.h"
#include <algorithm>
#include <iterator>
#include <memory>
#include <sys/_stdint.h>

namespace dsp_ui {

void APRSView::init() {

    set_font((FontDef *)&Font_7x10);

    add_children({&table_view, &console, &title_widget});

    title_widget.set_label("APRS");
    console.set_font(this->font);
    table_view.set_font(this->font);

    table_view.on_select = [this](APRSSource source) {
        this->on_source_selected(source);
    };

    // record_view.set_sampling_rate(24000);

    // baseband::set_aprs(1200);

    // audio::set_rate(audio::Rate::Hz_24000);
    // audio::output::start();

    // receiver_model.enable();

    aprs_signal.add(this, [this](void *, void *data) {
        on_packet((APRSPacket *)data);
    });

    dsp_command({(DSP_COMMAND)DSP_COMMAND_START, DSP_TASK_RECEIVE, &receive_task}, nullptr);
    main_board::setMode(DIGITAL_RX);
}

void APRSView::on_source_selected(APRSSource &source) {
}

void APRSView::before_paint(){

};

void APRSView::on_packet(APRSPacket *packet) {

    uint8_t ix = table_view.on_packet(packet);

    std::string str_console = "\x1B";

    std::string stream_text = packet->get_stream_text();
    str_console += (char)(ix);
    str_console += stream_text + "\n\n";

    console.write(str_console);
}

void APRSTableWidget::paint_callback() {

    // const RecentEntriesColumns columns{{{"Source", 9}, {"Loc", 6}, {"Hits", 4}, {"Time", 8}}};

    // Color target_color;
    // auto entry_age = entry.age;

    //  target_color = Theme::getInstance()->fg_green->foreground;
    char buf[100];

    display->clear();

    display->setFont(this->font);
    display->set_trim_enabled(false);
    display->setColor(C565_GREY_DARKER);

    int title_height = this->font->height + 5;
    display->drawRoundedRectangle(0, 0, area.box.width, title_height, 2, true);

    display->gotoXY(4, 3);
    sprintf(buf, "%-7s %5s %-8s\n", "Source", "Hits", "Time");
    display->setBgColor(C565_GREY_DARKER);
    display->setColor(C565_WHITE);
    display->print(buf);

    display->setBgColor(C565_BLACK);

    int i = 0;
    for (auto source : sources) {

        display->gotoXY(4, 5 + (++i * (font->height + 2)));
        display->setColor(palette16[i - 1]);
        sprintf(buf, "%-7s %s%4d %-8s\n", source.source_formatted.c_str(), source.hits <= 999 ? " " : "+", source.hits <= 999 ? source.hits : 999,
                source.time_string.c_str());

        display->print(buf);

        if (source.has_position) {
            // draw map icon
        }
    }
}

void APRSTableWidget::before_paint(){};

bool APRSTableWidget::on_touch(const st_inputEvent) {
    return true;

    //   recent_entries_view.on_select = [this](const APRSRecentEntry &entry) {
    //     this->on_show_detail(entry);
    // };
}

void APRSTableWidget::init() {
}

uint8_t APRSTableWidget::on_packet(APRSPacket *packet) {

    std::string source_formatted = packet->get_source_formatted();
    std::string info_string = packet->get_stream_text();

#if APRS_DEBUG
    info_string = "info from source " + source_formatted;
#endif

    auto it = std::find_if(sources.begin(), sources.end(), [&](const APRSSource &s) {
        return s.source == packet->get_source();
    });

    APRSSource *entry;
    int index;
    if (it != sources.end()) {
        entry = &(*it);
        index = std::distance(sources.begin(), it) - 1;
    } else {
        sources.emplace_back(packet->get_source());
        entry = &sources.back();
        index = max_sources - 1;
    }

    st_datetime dt = rtc_get_date_time();
    uint32_t ts = rtc_to_epoch(&dt.time, &dt.date);
    entry->age = ts;
    entry->hits++;

    entry->time_string = rtc_to_string(dt, true);
    entry->info_string = info_string;

    entry->source_formatted = source_formatted;

    if (entry->has_position && !packet->has_position()) {
        // maintain position info
    } else {
        entry->has_position = packet->has_position();
        entry->pos = packet->get_position();
    }

    // Sort by age
    std::sort(sources.begin(), sources.end(), [&](const APRSSource &a, const APRSSource &b) {
        return a.age > b.age;
    });

    if (sources.size() > max_sources) {
        sources.erase(sources.begin() + max_sources, sources.end());
    }

    if (on_select) {
        on_select(*entry);
    }

    set_dirty();

    return index;
}

} // namespace dsp_ui
