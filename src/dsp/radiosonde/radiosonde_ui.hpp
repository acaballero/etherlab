#ifndef __UI_SONDE_H__
#define __UI_SONDE_H__

//#include "ui_qrcode.hpp"
#include "Display_afb.h"
#include "ui/locator_view.h"
#include "ui/main_view.h"
#include "io/log_file.h"
#include "ui/map_view.h"
#include "radiosonde_packet.hpp"
#include "radiosonde_task.hpp"
#include <cstddef>
#include <string>

namespace dsp_ui {

#define RADIOSONDE_START_FREQ 405800000

class RadiosondeView : public View {
  public:
    static constexpr uint32_t initial_target_frequency = 402700000;

    RadiosondeView(Rect parent_rect) : View(parent_rect) {
        init();
    }

    void on_packet(radiosonde::Packet *packet);

  private:
    static constexpr int line_height = 14;
    static constexpr int c_width = 7;
    dsp::RadiosondeTask radiosonde_task{dspSuccess, dspError};

    std::unique_ptr<LogFile> logger;

    std::unique_ptr<radiosonde::Packet> curr_packet;
    //   RxRadioState radio_state_{
    //      402700000 /* frequency */, 1750000 /* bandwidth */, 2457600 /* sampling rate */
    // };
    bool enable_log{true};
    bool enable_crc{true};

    radiosonde::GPS_data gps_info{};
    radiosonde::temp_humid temp_humid_info{};
    std::string sonde_id{};

    Label title_widget{{0, 0, DISPLAY_X_PIXELS, 20}, C565_WHITE, C565_GREY_DARKER, ButtonStyle::BUTTON_STYLE_FLAT};

    Label lblType = {{4 * c_width, 2 * line_height}, "Type:", C565_TEXT_FG};
    Label lblId = {{6 * c_width, 3 * line_height}, "ID:", C565_TEXT_FG};
    Label lblDate = {{0 * c_width, 4 * line_height}, "DateTime:", C565_TEXT_FG};

    Label lblBatt = {{3 * c_width, 5 * line_height}, "Vbatt:", C565_TEXT_FG};
    Label lblFrame = {{3 * c_width, 6 * line_height}, "Frame:", C565_TEXT_FG};
    Label lblTemp = {{4 * c_width, 7 * line_height}, "Temp:", C565_TEXT_FG};
    Label lblHumidity = {{0 * c_width, 8 * line_height}, "Humidity:", C565_TEXT_FG};

    ui::Locator geopos{{DISPLAY_X_PIXELS / 2, 2 * line_height}, ui::Locator::alt_unit::METERS, ui::Locator::spd_unit::HIDDEN};

    std::unique_ptr<ui::MapView> map;

    Menu::menu_action_st menu_actions[6] = {{"CRC",

                                             [this]() {
                                                 enable_crc = !enable_crc;
                                                 menu_actions[0].enabled = enable_crc;
                                                 Menu::actions_signal.emit(&actions);
                                             }},
                                            {"Log",

                                             [this]() {
                                                 enable_log = !enable_log;
                                                 menu_actions[1].enabled = enable_log;
                                                 Menu::actions_signal.emit(&actions);
                                             }},
                                            {"Map",
                                             [this]() {
                                                 open_map();
                                             }},

                                            {"Exit", [this]() {
                                                 exit();
                                             }}};

    Menu::menu_actions_st actions = {menu_actions, sizeof(menu_actions) / sizeof(Menu::menu_action_st)};

    void on_freqchg(int64_t freq);

    void init();
    void exit();
    void start_rx();
    void open_map();

    void before_paint() override;

    MODE previous_mode;
    uint16_t previous_waterfall_speed;

    SignalToken radiosonde_signal_token;

    // void on_gps(const GPSPosDataMessage *msg);
};

} // namespace dsp_ui

#endif /*__UI_SONDE_H__*/
