#include "radiosonde_ui.hpp"

#include "dsp/radiosonde/radiosonde_task.hpp"

#include <cstring>
#include <stdio.h>
#include "dsp/dsp_tasks.h"
#include "input/inputEvent.h"
#include "ips_font.h"
#include "main_board.h"
#include "string_format.hpp"
#include "os/task_manager.h"

namespace dsp_ui {

using namespace dsp;

void RadiosondeView::init() {

    logger = std::make_unique<LogFile>();
    if (logger) {
        logger->append("radiosonde.log");
    }

    set_font((FontDef *)&Font_7x10);
    title_widget.set_label("Radiosonde tracker");

    add_children({&title_widget, &lblType, &lblId, &lblDate, &lblBatt, &lblFrame, &lblTemp, &lblHumidity, &geopos});

    geopos.set_read_only(true);

    radiosonde_signal_token = radiosonde_signal.add(this, [this](void *, const void *data) {
        on_packet((radiosonde::Packet *)data);
    });

    // gps_signal_token = gps_signal.add(this, [this](void *, void *data) {
    //     on_gps((gps::Packet *)data);
    // });

    actions.actions[0].enabled = enable_crc;
    actions.actions[1].enabled = enable_log;

    actions_signal.emit(&actions);

    // Get some current parameters so they can be restored on exit
    previous_mode = config.mode;
    previous_waterfall_speed = config.fft.waterfall_pixels_per_second;

    fft::set_waterfall_speed(1);

    radio::set_band(radio::BAND_AUTO); // must do this in case we are band-limited
    radio::set_frequency(RADIOSONDE_START_FREQ);

    start_rx();
}

void RadiosondeView::open_map() {

    if (curr_packet) {
        if (map) {
            map.reset();
        }
        map = std::make_unique<ui::MapView>(curr_packet->serial_number(), -1, ui::Locator::alt_unit::METERS, ui::Locator::spd_unit::HIDDEN,
                                            curr_packet->get_GPS_data().lat, curr_packet->get_GPS_data().lon, 0, [this]() {
                                                view_manager::mainView.remove_child(map.get());
                                                //   table_view.set_visible(true);
                                            });

        view_manager::mainView.add_child(map.get());
        view_manager::mainView.to_top(map.get());
        map->set_focus(true);
    }
}

void RadiosondeView::exit() {

    dsp_command({(DSP_COMMAND)DSP_COMMAND_STOP, dsp::DSP_TASK_RECEIVE, &radiosonde_task}, [this](st_dsp_params *status) {
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

void RadiosondeView::before_paint() {
    display->setFont((FontDef *)&Font_7x10);
}

void RadiosondeView::start_rx() {
    //  LOG("START RX\n");
    dsp_command({(DSP_COMMAND)DSP_COMMAND_START, dsp::DSP_TASK_RECEIVE, &radiosonde_task}, [this](st_dsp_params *status) {
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

void RadiosondeView::on_packet(radiosonde::Packet *packet) {
    if (enable_crc && !packet->crc_ok()) {
        return;
    }

    curr_packet = std::make_unique<radiosonde::Packet>(*packet);

    lblType.set_value(packet->type_string().c_str());

    sonde_id = packet->serial_number(); // used also as tag on the geomap
    lblId.set_value(sonde_id.c_str());

    char rtc_str[20];
    rtc_to_string(packet->received_at(), false, rtc_str);
    lblDate.set_value(rtc_str);

    lblBatt.set_value((unit_auto_scale(packet->battery_voltage(), 2, 2) + "V").c_str());

    lblFrame.set_value(to_string_dec_uint(packet->frame(), 0).c_str()); // euquiq: integrate frame #, temp & humid.

    temp_humid_info = packet->get_temp_humid();
    if (temp_humid_info.humid != 0) {
        double decimals = abs(get_decimals(temp_humid_info.humid, 10, true));
        lblHumidity.set_value((to_string_dec_int((int)temp_humid_info.humid) + "." + to_string_dec_uint(decimals, 1) + "%").c_str());
    }

    if (temp_humid_info.temp != 0) {
        double decimals = abs(get_decimals(temp_humid_info.temp, 10, true));
        lblTemp.set_value((to_string_dec_int((int)temp_humid_info.temp) + "." + to_string_dec_uint(decimals, 1) + "º").c_str());
    }

    gps_info = packet->get_GPS_data();

    geopos.set_altitude(gps_info.alt);
    geopos.set_lat(gps_info.lat);
    geopos.set_lon(gps_info.lon);

    if (enable_log) {
        const auto formatted = packet->symbols_formatted();
        logger->log(formatted.data);
    }
}

} // namespace dsp_ui
