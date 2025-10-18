#include "radiosonde_ui.hpp"

#include "dsp/radiosonde/radiosonde_task.hpp"

#include <cstring>
#include <stdio.h>
#include "dsp/dsp_tasks.h"
#include "input/inputEvent.h"
#include "main_board.h"
#include "string_format.hpp"
#include "os/task_manager.h"

namespace dsp_ui {

void RadiosondeView::init() {

    logger = std::make_unique<LogFile>();
    if (logger) {
        logger->append("radiosonde.log");
    }

    add_children({&lblType, &lblId, &lblDate, &lblBatt, &lblFrame, &lblTemp, &lblHumidity, &geopos, &button_see_map});

    geopos.set_read_only(true);

    button_see_map.action = [this](Button &, st_inputEvent) {
        if (map) {
            map.reset();
        }
        map = std::make_unique<ui::MapView>(std::string(source.source_formatted), -1, ui::Locator::alt_unit::METERS, ui::Locator::spd_unit::HIDDEN,
                                            source.pos.latitude, source.pos.longitude, 0, [this]() {
                                                view_manager::mainView.remove_child(map.get());
                                                //   table_view.set_visible(true);
                                            });

        view_manager::mainView.add_child(map.get());
        view_manager::mainView.to_top(map.get());
        map->set_focus(true);
    };

    radiosonde_signal_token = radiosonde_signal.add(this, [this](void *, void *data) {
        on_packet((radiosonde::Packet *)data);
    });

    // gps_signal_token = gps_signal.add(this, [this](void *, void *data) {
    //     on_gps((gps::Packet *)data);
    // });

    actions_signal.emit(&actions);

    // Get some current parameters so they can be restored on exit
    previous_mode = config.mode;
    previous_waterfall_speed = config.fft.waterfall_pixels_per_second;

    fft::set_waterfall_speed(1);

    radio::set_band(radio::BAND_AUTO); // must do this in case we are band-limited
    radio::set_frequency(EU_RADIOSONDE_FREQ);

    start_rx();
}

void RadiosondeView::exit() {

    dsp_command({(DSP_COMMAND)DSP_COMMAND_STOP, dsp::DSP_TASK_RECEIVE, &radiosonde_task}, [this](st_dsp_status *status) {
        if (status->status == DSP_STATUS_STOPPED) {

            radiosonde_signal.remove(radiosonde_signal_token);

            dsp::set_agc_enabled(true); // Turn on AGC

            MODE m = previous_mode;
            uint16_t ws = previous_waterfall_speed;

            os::task_manager.set_timeout(1, [m, ws]() {
                if (m == DIGITAL_RX) {
                    dsp_command({(DSP_COMMAND)DSP_COMMAND_START, dsp::DSP_TASK_RECEIVE}, nullptr);
                }
                //  LOG("Fired delayed close of APRS view\n");
                main_board::set_mode(m);

                fft::set_waterfall_speed(ws);

                // Re-enable analog mute
                main_board::enable_analog_mute(true);
            });

            // Clear specific bottom quick buttons
            actions_signal.emit(nullptr);

            set_visible(false);
        }
    });
}

void RadiosondeView::start_rx() {
    //  LOG("START RX\n");
    dsp_command({(DSP_COMMAND)DSP_COMMAND_START, dsp::DSP_TASK_RECEIVE, &radiosonde_task}, [this](st_dsp_status *status) {
        if (status->status == DSP_STATUS_STOPPED) {
            if (status->error != DSP_ERR_NONE) {
                exit();
                status::pop_alert(status::ERROR, "Error starting Radiosonde task");
            }
        }
    });

    // To execute a task other than DSP_TASK_RECEIVE, set_mode has to be called
    main_board::set_mode(DIGITAL_RX);

    dsp::set_agc_enabled(false); // Prevent sudden changes in gain from the digital AGC. TODO: Whether digital AGC is enabled or not should be a property of the
                                 // modulation mode (create one for digital modes)

    // Disable analog mute. Squelch is done digitally
    main_board::enable_analog_mute(false);

    // Set the configured IF gain (otherwise having disabled AGC it can be whatever not appropriate)
    // TODO: It would be better to leave the AGC enabled, but setting
    // long release and  small attack times. However, a digital packet burst is very short and react quickly enough quite difficult in the current platform
    if_gain(RF_DIRECTION_RX, IF_GAIN_MINUS18, config.hw.cmx973_vgb);
}

// void RadiosondeView::on_gps(const GPSPosDataMessage *msg) {
//     if (!geomap_view_)
//         return;
//     geomap_view_->update_my_position(msg->lat, msg->lon, msg->altitude);
// }

void RadiosondeView::toggle_crc() {

    enable_crc = !enable_crc;
    if (enable_crc) {
        menu_actions[0].bg_color = C565_BG_ENABLED;
        menu_actions[0].fg_color = C565_GREEN;

    } else {
        menu_actions[0].fg_color = C565_TEXT_FG;
        menu_actions[0].bg_color = C565_BG_DISABLED;
    }

    actions_signal.emit(&actions);
}

void RadiosondeView::toggle_log() {

    enable_log = !enable_log;
    if (enable_log) {
        menu_actions[1].bg_color = C565_BG_ENABLED;
        menu_actions[1].fg_color = C565_GREEN;

    } else {
        menu_actions[1].fg_color = C565_TEXT_FG;
        menu_actions[1].bg_color = C565_BG_DISABLED;
    }

    actions_signal.emit(&actions);
}

void RadiosondeView::on_packet(radiosonde::Packet *packet) {
    if (!enable_crc || packet->crc_ok()) // euquiq: Reject bad packet if crc is on
    {
        lblType.set_value(packet->type_string().c_str());

        sonde_id = packet->serial_number(); // used also as tag on the geomap
        lblId.set_value(sonde_id.c_str());

        lblDate.set_value(packet->received_at());

        lblBatt.set_value((unit_auto_scale(packet->battery_voltage(), 2, 2) + "V").c_str());

        lblFrame.set_value(to_string_dec_uint(packet->frame(), 0).c_str()); // euquiq: integrate frame #, temp & humid.

        temp_humid_info = packet->get_temp_humid();
        if (temp_humid_info.humid != 0) {
            double decimals = abs(get_decimals(temp_humid_info.humid, 10, true));
            lblHumidity.set_value((to_string_dec_int((int)temp_humid_info.humid) + "." + to_string_dec_uint(decimals, 1) + "%").c_str());
        }

        if (temp_humid_info.temp != 0) {
            double decimals = abs(get_decimals(temp_humid_info.temp, 10, true));
            lblTemp.set_value((to_string_dec_int((int)temp_humid_info.temp) + "." + to_string_dec_uint(decimals, 1) + STR_DEGREES_C).c_str());
        }

        gps_info = packet.get_GPS_data();

        geopos.set_altitude(gps_info.alt);
        geopos.set_lat(gps_info.lat);
        geopos.set_lon(gps_info.lon);

        if (enable_log) {
            logger->append(packet->crc_ok());
        }
    }
}

} // namespace dsp_ui
