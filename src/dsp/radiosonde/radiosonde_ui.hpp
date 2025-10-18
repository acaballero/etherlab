#ifndef __UI_SONDE_H__
#define __UI_SONDE_H__

//#include "ui_qrcode.hpp"
#include "ui/locator_view.h"
#include "ui/main_view.h"
#include "io/log_file.h"
#include "ui/map_view.h"
#include "radiosonde_packet.hpp"
#include "radiosonde_task.hpp"
#include <cstddef>
#include <string>

namespace dsp_ui {

class RadiosondeView : public View {
  public:
    static constexpr uint32_t initial_target_frequency = 402700000;

    RadiosondeView(Rect parent_rect) : View(parent_rect) {
        init();
    }

    void on_packet(radiosonde::Packet *packet);

  private:
    RadiosondeTask radiosonde_task{dspSuccess, dspError};

    std::unique_ptr<LogFile> logger;

    //   RxRadioState radio_state_{
    //      402700000 /* frequency */, 1750000 /* bandwidth */, 2457600 /* sampling rate */
    // };
    bool enable_log{false};
    bool enable_crc{false};

    radiosonde::GPS_data gps_info{};
    radiosonde::temp_humid temp_humid_info{};
    std::string sonde_id{};

    // AudioOutput audio_output { };

    Label lblType = {{4 * 8, 2 * 16}, "Type:"};
    Label lblId = {{6 * 8, 3 * 16}, "ID:"};
    Label lblDate = {{0 * 8, 4 * 16}, "DateTime:"};

    Label lblBatt = {{3 * 8, 5 * 16}, "Vbatt:"};
    Label lblFrame = {{3 * 8, 6 * 16}, "Frame:"};
    Label lblTemp = {{4 * 8, 7 * 16}, "Temp:"};
    Label lblHumidity = {{0 * 8, 8 * 16}, "Humidity:"};

    ui::Locator geopos{{0, 12 * 16}, ui::Locator::alt_unit::METERS, ui::Locator::spd_unit::HIDDEN};

    Button button_see_map{{16 * 8, 15 * 16, 12 * 8, 3 * 16}, &lcd, "See on map"};

    std::unique_ptr<ui::MapView> map;

    Menu::menu_action_st menu_actions[6] = {{"CRC",
                                             [this]() {
                                                 toggle_crc();
                                             }},
                                            {"Log",
                                             [this]() {
                                                 toggle_log();
                                             }},

                                            {"Exit", [this]() {
                                                 exit();
                                             }}};

    Menu::menu_actions_st actions = {menu_actions, sizeof(menu_actions) / sizeof(Menu::menu_action_st)};

    void on_freqchg(int64_t freq);

    void toggle_crc();
    void toggle_log();

    void init();
    void exit();
    void start_rx();

    MODE previous_mode;
    uint16_t previous_waterfall_speed;

    SignalToken radiosonde_signal_token;

    // void on_gps(const GPSPosDataMessage *msg);
};

} // namespace dsp_ui

#endif /*__UI_SONDE_H__*/
