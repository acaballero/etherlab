//
// Created by Angel Dust on 18/05/2025.
//
#include "aprs_ui.h"
#include "Display_afb.h"
#include "dsp/aprs/aprs_packet.h"
#include "dsp/aprs/aprs_rx_task.h"
#include "dsp/dsp_common.h"
#include "dsp/dsp_tasks.h"
#include "hw/stm32f4xx/rtc.h"
#include "input/inputEvent.h"
#include "ips_font.h"
#include "main_board.h"
#include "os/periodic_task.h"
#include "os/task_manager.h"
#include "radio.h"
#include "status.h"
#include "types.h"
#include "ui/console_widget.h"
#include <cstddef>
#include <cstring>
#include <iterator>
#include <string>
#include <sys/_stdint.h>
#include "dsp/protocols/aprs.hpp"

namespace dsp_ui {

void APRSView::init() {

    set_font((FontDef *)&Font_7x10);
    title_widget.set_label("APRS");

    add_children({&table_view, &console, &title_widget});

    console.set_font(this->font);
    table_view.set_font(this->font);

    table_view.set_focus(true);

    table_view.on_select = [this](APRSSource &source) {
        this->on_source_selected(source);
    };

    aprs_signal_token = aprs_signal.add(this, [this](void *, void *data) {
        on_packet((APRSPacket *)data);
    });

    actions_signal.emit(&actions);

    previous_mode = config.mode;

    radio::set_frequency(EU_APRS_FREQ);

    start_rx();
}

void APRSView::stop() {
    menu_actions[0].name = "Resume";
    paused = true;
    actions_signal.emit(&actions);
}
void APRSView::resume() {
    menu_actions[0].name = "Pause";
    paused = false;
    actions_signal.emit(&actions);
}

void APRSView::toggle_beacon() {
    auto *p = new os::periodic_task{5000, [this]() {
                                        send_packet("Beacon");
                                    }};

    if (!os::task_manager.remove(beacon_task_id)) {
        menu_actions[2].bg_color = C565_BG_ENABLED;
        menu_actions[2].fg_color = C565_GREEN;
        beacon_task_id = os::task_manager.add(p);
    } else {
        menu_actions[2].fg_color = C565_TEXT_FG;
        menu_actions[2].bg_color = C565_BG_DISABLED;
    }

    actions_signal.emit(&actions);
}
void APRSView::start_rx() {
    dsp_command({(DSP_COMMAND)DSP_COMMAND_START, DSP_TASK_RECEIVE, &aprs_task}, nullptr);
    // To execute a task other than DSP_TASK_RECEIVE, setMode has to be called so
    main_board::setMode(DIGITAL_RX);
}

void APRSView::settings() {

    view_manager::keyboardView.set_text("");
    view_manager::keyboardView.set_label("Info");
    view_manager::keyboardView.set_size(255);
    view_manager::keyboardView.on_changed = [this](char *str) {
        send_packet(std::string(str));
    };
    view_manager::push((View *)&view_manager::keyboardView);
}

void APRSView::exit() {

    dsp_command({(DSP_COMMAND)DSP_COMMAND_STOP, DSP_TASK_RECEIVE, &aprs_task}, [this](st_dsp_status *status) {
        if (status->status == DSP_STATUS_STOPPED) {

            os::task_manager.remove(beacon_task_id);

            aprs_signal.remove(aprs_signal_token);

            MODE m = previous_mode;
            os::task_manager.set_timeout(1, [m]() {
                if (m == DIGITAL_RX) {
                    dsp_command({(DSP_COMMAND)DSP_COMMAND_START, DSP_TASK_RECEIVE}, nullptr);
                }
                main_board::setMode(m);
            });

            // Clear specific bottom quick buttons
            actions_signal.emit(nullptr);

            set_visible(false);
        }
    });
}

bool APRSView::on_input(const st_inputEvent e) {
    bool consumed = Widget::on_input(e);

    if (consumed) {
        return true;
    }

    switch (e.type) {

        case INPUT_EVENT_TYPE_BUTTON_PRESS:
        case INPUT_EVENT_TYPE_BUTTON_DBL_PRESS:

            switch (e.value) {
                case KEY_BACK:
                    exit();
                    break;
                default:
                    consumed = false;
            }
            break;
        case INPUT_EVENT_TYPE_BUTTON_RELEASE:

            switch (e.value) {
                case KEY_BACK:
                    consumed = true;
                    break;
                default:
                    consumed = false;
            }

            break;
        default:
            consumed = false;
            break;
    }

    return consumed;
}

void APRSView::on_source_selected(APRSSource &source) {
    current_source = source;
}

void APRSView::before_paint() {
}

void APRSView::send_packet(std::string info) {

    uint16_t buffer[256];
    trim(config.callsign);
    aprs::build_frame(config.callsign, 0, "rig   ", 0, ":" + info, buffer);

    aprs_tx_task.configure(1200, 2200, 1, 8, 10000, 200, 100); // APRS uses fixed 10k bandwidth
    aprs_tx_task.set_data(buffer);

    dsp_command({(DSP_COMMAND)DSP_COMMAND_START, DSP_TASK_REPLAY, &aprs_tx_task}, [this](st_dsp_status *status) {
        if (status->status == DSP_STATUS_STOPPED) {
            if (status->fifo_underruns) {
                status::handleError(status::ST_ERROR, "FIFO underruns");
            }
            start_rx();
        }
    });

    // To execute a task other than DSP_TASK_REPLAY, setMode has to be called so
    main_board::setMode(DIGITAL_TX);
}

void APRSView::on_packet(APRSPacket *packet) {

    if (paused) {
        return;
    }

    int ix = table_view.on_packet(packet);

    std::string str_console = {ConsoleWidget::color_mark};

    std::string stream_text;
    packet->get_stream_text(stream_text);
    str_console += (char)(ix + 1); // Colors index starts in 1
    str_console += stream_text + "\n";

    console.write(str_console);
}

} // namespace dsp_ui
