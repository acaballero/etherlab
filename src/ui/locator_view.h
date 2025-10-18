//
// Created by Angel Dust on 18/10/2025.
//

#ifndef LOCATOR_VIEW_H
#define LOCATOR_VIEW_H

#include "number_field_widget.h"
#include "label_widget.h"
#include "view.h"

namespace ui {
class Locator : public View {
  public:
    enum alt_unit { FEET = 0, METERS };
    enum spd_unit { NONE = 0, MPH, KMPH, HIDDEN = 255 };

    std::function<void(int32_t, float, float, int32_t)> on_change{};

    Locator(const Point pos, const alt_unit altitude_unit, const spd_unit speed_unit, FontDef *f = (FontDef *)&Font_7x10);

    void on_focus() override;

    void set_read_only(bool v);
    void set_altitude(int32_t altitude);
    void set_speed(int32_t speed);
    void set_lat(float lat);
    void set_lon(float lon);
    int32_t altitude();
    int32_t speed();
    void hide_altandspeed();
    float lat();
    float lon();

    void set_report_change(bool v);

    void before_paint() override{};

  private:
    static constexpr uint8_t c_width = 7; // Make it match font width
    static constexpr uint8_t margin = 4;
    static constexpr uint8_t c_height = 10 + margin;
    bool read_only{false};
    bool report_change{true};
    alt_unit altitude_unit_{};
    spd_unit speed_unit_{};

    Label label_alt{{1 * c_width, 0 * c_height}, "Alt:", C565_WHITE};
    NumberField field_altitude{{6 * c_width, 0 * c_height}, 5, {-1000, 50000}, 1, " m"};

    Label label_lat{{1 * c_width, 1 * c_height}, "Lat:", C565_WHITE};
    Label label_lon{{1 * c_width, 2 * c_height}, "Lon:", C565_WHITE};

    Label label_spd_position{{15 * c_width, 0 * c_height}, "Spd:", C565_WHITE};
    NumberField field_speed{{20 * c_width, 0 * c_height}, 4, {0, 5000}, 1, " Km/h"};

    NumberField field_lat_degrees{{7 * c_width, 1 * c_height}, 4, {-90, 90}, 1, " "};
    NumberField field_lat_minutes{{12 * c_width, 1 * c_height}, 2, {0, 59}, 1, "'", true};
    NumberField field_lat_seconds{{15 * c_width, 1 * c_height}, 2, {0, 59}, 1, "''", true};
    Label text_lat_decimal{{19 * c_width, 1 * c_height, 13 * c_width, 1 * c_height}, C565_GREY_DARK};

    NumberField field_lon_degrees{{7 * c_width, 2 * c_height}, 4, {-180, 180}, 1, " "};
    NumberField field_lon_minutes{{12 * c_width, 2 * c_height}, 2, {0, 59}, 1, "'", true};
    NumberField field_lon_seconds{{15 * c_width, 2 * c_height}, 2, {0, 59}, 1, "''", true};
    Label text_lon_decimal{{{19 * c_width, 2 * c_height}, {13 * c_width, 1 * c_height}}, C565_GREY_DARK};
};
} // namespace ui

#endif // LOCATOR_VIEW_H
