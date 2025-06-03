//
// Created by Angel Dust on 18/05/2025.
//
#include "aprs_ui.h"
#include "Display_afb.h"
#include "dsp/aprs/aprs_packet.h"
#include "dsp/aprs/aprs_task.h"
#include "dsp/dsp_common.h"
#include "dsp/dsp_tasks.h"
#include "hw/stm32f4xx/rtc.h"
#include "ips_font.h"
#include "main_board.h"
#include "types.h"
#include "ui/console_widget.h"
#include "ui/lcd.h"
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <memory>
#include <string>
#include <sys/_intsup.h>
#include <sys/_stdint.h>

namespace dsp_ui {

void APRSView::init() {

    set_font((FontDef *)&Font_7x10);
    title_widget.set_label("APRS");

    add_children({&table_view, &console, &title_widget});

    console.set_font(this->font);
    table_view.set_font(this->font);

    table_view.on_select = [this](APRSSource &source) {
        this->on_source_selected(source);
    };

    // record_view.set_sampling_rate(24000);

    // baseband::set_aprs(1200);

    // audio::set_rate(audio::Rate::Hz_24000);
    // audio::output::start();

    // receiver_model.enable();

    aprs_signal_token = aprs_signal.add(this, [this](void *, void *data) {
        on_packet((APRSPacket *)data);
    });

    actions_signal.emit(&actions);

    previous_mode = config.mode;

    dsp_command({(DSP_COMMAND)DSP_COMMAND_START, DSP_TASK_RECEIVE, &receive_task}, nullptr);
    // To execute a task other than DSP_TASK_RECEIVE, setMode has to be called so
    main_board::setMode(DIGITAL_RX);
}

void APRSView::exit() {

    dsp_command({(DSP_COMMAND)DSP_COMMAND_STOP, DSP_TASK_RECEIVE, &receive_task}, [this](st_dsp_status *status) {
        if (status->status == DSP_STATUS_STOPPED) {
            aprs_signal.remove(aprs_signal_token);
            actions_signal.emit(nullptr);
            set_visible(false);
            main_board::setMode(previous_mode);
        }
    });
}

void APRSView::on_source_selected(APRSSource &source) {
}

void APRSView::before_paint(){

};

void APRSView::on_packet(APRSPacket *packet) {

    int ix = table_view.on_packet(packet);

    std::string str_console = {ConsoleWidget::color_mark};

    std::string stream_text;
    packet->get_stream_text(stream_text);
    str_console += (char)(ix + 1); // Colors index starts in 1
    str_console += stream_text + "\n\n";

    console.write(str_console);
}

void APRSTableWidget::paint_callback() {

    char buf[26];

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

    for (size_t i = 0; i < sources.size(); i++) {
        APRSSource *source = &sources[i];

        display->gotoXY(4, 5 + ((i + 1) * (font->height + 2)));

        display->setColor(palette16[source->id]);

        snprintf(buf, sizeof(buf), "%-7s %s%4d %-8s\n", source->source_formatted, source->hits <= 999 ? " " : "+", source->hits <= 999 ? source->hits : 999,
                 source->time_string);

        display->print(buf);

        if (source->has_position) {
            // draw map icon
        }
    }
}

void APRSTableWidget::before_paint(){};

bool APRSTableWidget::on_touch(const st_inputEvent) {
    return true;

    //   recent_entries_view.on_select = [this](const APRSRecentEntry &entry) {
    //     this->on_show_detail(entry);
    //
    //  if (on_select) {
    //    on_select(*entry);
    //}
    //};
}

void APRSTableWidget::init() {
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

int APRSTableWidget::on_packet(APRSPacket *packet) {

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
        // maintain position info
    } else {
        //    entry.has_position = packet->has_position();
        //       entry.pos = packet->get_position();
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
