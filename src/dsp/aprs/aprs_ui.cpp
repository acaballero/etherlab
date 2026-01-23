//
// Created by Angel Dust on 18/05/2025.
//
#include "aprs_ui.h"
#include "Display_afb.h"
#include "arm_math.h"
#include "beacon_settings_view.h"
#include "dsp/aprs/aprs_packet.h"
#include "dsp/aprs/aprs_rx_task.h"
#include "dsp/aprs/aprs_settings.h"
#include "dsp/dsp_common.h"
#include "dsp/dsp_processors.h"
#include "dsp/dsp_tasks.h"
#include "dsp/fft/fft.h"
#include "hw/board/board_v2.h"
#include "hw/stm32f4xx/rtc.h"
#include "input/inputEvent.h"
#include "io/config_file.h"
#include "io/log_file.h"
#include "ips_font.h"
#include "main_board.h"
#include "os/periodic_task.h"
#include "os/task_manager.h"
#include "radio.h"
#include "s_strength.h"
#include "status.h"
#include "memory.h"
#include "types.h"
#include "ui/console_widget.h"
#include <cstddef>
#include <cstring>
#include <iterator>
#include <string>

#include "dsp/protocols/aprs.hpp"
#include "ui/map_view.h"
#include "ui/menu_options.h"
#include "ui/ui_types.h"
#include "ui/view_manager.h"
#include "ui/widget.h"

namespace dsp_ui {
using Menu::navigation_signal;

void APRSView::init() {

    logger = std::make_unique<LogFile>();
    set_name("aprs");
    if (logger) {
        logger->append("aprs.log");
    }

    set_font((FontDef *)&Font_7x10);
    title_widget.set_label("APRS");
    title_widget.set_border_radius(0, 0, 0, 0);

    button_collapse.set_aling(ALIGN_CENTER);

    button_collapse.action = [this](Button &, st_inputEvent) {
        collapsed = !collapsed;

        console.set_visible(!collapsed);

        if (collapsed) {
            set_width(METER_WIDTH);
            button_collapse.set_text(">>");
            button_collapse.set_left(METER_WIDTH - button_collapse_width - 1);
            title_widget.set_width(METER_WIDTH - button_collapse_width);

        } else {
            set_width(DISPLAY_X_PIXELS);
            button_collapse.set_text("<<");
            button_collapse.set_left(DISPLAY_X_PIXELS - button_collapse_width - 1);
            title_widget.set_width(DISPLAY_X_PIXELS - button_collapse_width);
        }
    };

    table_view.set_z_index(100);

    add_children({&table_view, &console, &title_widget, &button_collapse});

    if (config.debug) {
        table_view.set_height(table_view.parent_rect().height() - title_height - 2);
        gain.set_top(parent_rect().height() - title_height);
        add_child(&gain);
    }

    console.set_font(this->font);
    table_view.set_font(this->font);

    table_view.set_focus(true);

    table_view.on_select = [this](APRSSource &source) {
        this->on_source_selected(source);
    };

    aprs_signal_token = aprs_signal.add(this, [this](void *, const void *data) {
        on_packet((APRSPacket *)data);
    });

    // Get some current parameters so they can be restored on exit
    previous_mode = config.mode;
    previous_waterfall_speed = config.fft.waterfall_pixels_per_second;

    fft::set_waterfall_speed(1);

    radio::set_band(radio::BAND_AUTO); // must do this in case we are band-limited
    radio::set_frequency(EU_APRS_FREQ);

    start_rx();
}

void APRSView::stop() {
    menu_actions[0].name = "Resume";
    paused = true;
    actions.dirty = true;
    navigation_signal.emit(this);
}

void APRSView::resume() {
    menu_actions[0].name = "Pause";
    paused = false;
    actions.dirty = true;
    navigation_signal.emit(this);
}

void APRSView::toggle_beacon() {

    if (!os::task_manager.remove(beacon_task_id)) {
        // Was disabled
        auto settings_view = std::make_unique<BeaconSettingsView>(

            [this](bool ok, aprs::settings settings) {
                if (ok) {

                    aprs_settings = settings;
                    auto *p = new os::periodic_task{static_cast<uint64_t>(settings.beacon_period_ms), [this]() {
                                                        send_packet(aprs_settings.message);
                                                    }};

                    menu_actions[2].bg_color = C565_GREEN;
                    menu_actions[2].fg_color = C565_WHITE;
                    beacon_task_id = os::task_manager.add(p);
                    actions.dirty = true;
                    navigation_signal.emit(this);
                }
            });

        view_manager::open(move(settings_view));
    } else {
        menu_actions[2].fg_color = C565_BUTTON_TEXT_FG;
        menu_actions[2].bg_color = C565_BG_DISABLED;
        actions.dirty = true;
        navigation_signal.emit(this);
    }
}

void APRSView::start_rx() {
    //  LOG("START RX\n");
    dsp_command({(DSP_COMMAND)DSP_COMMAND_START, dsp::DSP_PROCESSOR_RECEIVE, &aprs_task}, [this](st_dsp_params *status) {
        if (status->status == DSP_STATUS_STOPPED) {
            if (status->error != DSP_ERR_NONE) {
                exit();
                status::pop_alert(status::ERROR, "Error starting APRS task");
            }
        }
    });
    // To execute a task other than DSP_TASK_RECEIVE, set_mode has to be called
    main_board::set_mode(DIGITAL_RX);

    set_agc_enabled(false); // Prevent sudden changes in gain from the digital AGC. TODO: Whether digital AGC is enabled or not should be a property of the
                            // modulation mode (create one for digital modes)

    // Disable analog mute. Squelch is done digitally
    main_board::enable_analog_mute(false);

    // Set the configured IF gain (otherwise having disabled AGC it can be whatever not appropriate)
    // TODO: It would be better to leave the AGC enabled, but setting
    // long release and  small attack times. However, an APRS burst is very short and react quickly enough quite difficult in the current platform
    if_gain(RF_DIRECTION_RX, IF_GAIN_MINUS18, config.hw.cmx973_vgb);
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

void APRSView::threshold() {

    Menu::open_number_edit<int8_t>(
        aprs_task.get_bit_threshold(), "", "Bit threshold", 0,
        [this](int8_t v) {
            aprs_task.set_bit_threshold(v);
        },
        -128, 127, 1, 1);
}

void APRSView::exit() {

    dsp_command({(DSP_COMMAND)DSP_COMMAND_STOP, dsp::DSP_PROCESSOR_RECEIVE, &aprs_task}, [this](st_dsp_params *status) {
        if (status->status == DSP_STATUS_STOPPED) {

            os::task_manager.remove(beacon_task_id);

            aprs_signal.remove(aprs_signal_token);

            set_agc_enabled(true); // Turn on AGC

            MODE m = previous_mode;
            uint16_t ws = previous_waterfall_speed;

            os::task_manager.set_timeout(1, [m, ws]() {
                if (m == DIGITAL_RX) {
                    dsp_command({(DSP_COMMAND)DSP_COMMAND_START, dsp::DSP_PROCESSOR_RECEIVE}, nullptr);
                }
                //  LOG("Fired delayed close of APRS view\n");
                main_board::set_mode(m);

                fft::set_waterfall_speed(ws);

                // Re-enable analog mute
                main_board::enable_analog_mute(true);
            });

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

    if (current_source.has_position) {

        if (map) {
            map.reset();
        }
        map = std::make_unique<ui::MapView>(std::string(source.source_formatted), -1, ui::Locator::alt_unit::METERS, ui::Locator::spd_unit::HIDDEN,
                                            source.pos.latitude, source.pos.longitude, 0, [this]() {
                                                view_manager::mainView.remove_child(map.get());
                                                table_view.set_visible(true);
                                            });

        table_view.set_visible(false);
        view_manager::mainView.add_child(map.get());
        view_manager::mainView.to_top(map.get());
        map->set_focus(true);
    }
}

void APRSView::before_paint() {
}

void APRSView::send_packet(std::string info) {

    uint16_t buffer[256];
    trim(config.callsign);

    // APZxxx identifies an experimental rig/software
    size_t bytes = aprs::build_frame(config.callsign, 0, "APZ001", 0, info, aprs_settings.path, buffer);

    // DEBUG built frame by passing it through the RX chain
    // for (size_t i = 0; i < bytes; i++) {
    //     uint8_t nrzi_byte = static_cast<uint8_t>(buffer[i] & 0xFF);

    //     for (int b = 7; b >= 0; b--) {
    //         uint8_t nrzi_bit = (nrzi_byte >> b) & 1;

    //         if (aprs_task.parse_bit(nrzi_bit)) {
    //             aprs_task.parse_packet(); // validates CRC and calls parse_ax25() if OK
    //         }
    //     }
    // }

    LOG_IND(2, "Sending APRS packet: Address: %s | path: %s | payload: %s\n", config.callsign, aprs_settings.path, info.c_str());

    aprs_tx_task.configure(1200, 2200, 1, 8, aprs_settings.deviation, 300, 300); // Set a deviation for around 10k bandwidth
    aprs_tx_task.set_data(buffer);

    dsp_command({(DSP_COMMAND)DSP_COMMAND_START, DSP_TASK_REPLAY, &aprs_tx_task}, [this](st_dsp_params *status) {
        if (status->status == DSP_STATUS_STOPPED) {
            if (status->fifo_underruns) {
                status::pop_alert(status::ERROR, "FIFO underruns");
            }
            LOG_IND(-2, "Finished sending APRS packet\n");
            start_rx();
        }
    });

    // To execute a task other than DSP_TASK_REPLAY, setMode has to be called so
    main_board::set_mode(DIGITAL_TX);
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

    if (logger) {
        logger->log(stream_text);
    }

    console.write(str_console);
}

} // namespace dsp_ui
