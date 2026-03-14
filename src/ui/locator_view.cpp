//
// Created by Angel Dust on 18/10/2025.
//
#include "locator_view.h"

#include "utils.hpp"

namespace ui {

Locator::Locator(const Point pos, const alt_unit altitude_unit, const spd_unit speed_unit, FontDef *f)
    : View(), altitude_unit_(altitude_unit), speed_unit_(speed_unit) {

    set_focusable(true);

    set_font(f);
    set_parent_rect({pos.x(), pos.y(), (DISPLAY_X_PIXELS / 2), 3 * c_height});

    add_children({&label_alt, &label_lat, &label_lon, &label_spd_position, &text_lat_decimal, &text_lon_decimal});
    add_children({&field_altitude, &field_speed, &field_lat_degrees, &field_lat_minutes, &field_lat_seconds, &field_lon_degrees, &field_lon_minutes,
                  &field_lon_seconds});

    for (auto w : children()) {
        w->set_font(get_font());
    }

    // Defaults
    set_altitude(0);
    set_speed(0);
    set_lat(0);
    set_lon(0);

    const auto changed_fn = [this](int32_t) {
        // Convert degrees/minutes/seconds fields to decimal (floating point) lat/lon degree
        float lat_value = lat();
        float lon_value = lon();

        char buf[6];

        format_double(lat_value, buf, '.', ' ', 5);
        text_lat_decimal.set_label(buf);
        format_double(lon_value, buf, '.', ' ', 5);
        text_lon_decimal.set_label(buf);

        if (on_change && report_change) {
            on_change(altitude(), lat_value, lon_value, speed());
        }
    };

    field_altitude.on_change = changed_fn;
    field_speed.on_change = changed_fn;
    field_lat_degrees.on_change = changed_fn;
    field_lat_minutes.on_change = changed_fn;
    field_lat_seconds.on_change = changed_fn;
    field_lon_degrees.on_change = changed_fn;
    field_lon_minutes.on_change = changed_fn;
    field_lon_seconds.on_change = changed_fn;

    const auto wrapped_lat_seconds = [this](int32_t v) {
        field_lat_minutes.add(v);
    };

    const auto wrapped_lat_minutes = [this](int32_t v) {
        field_lat_degrees.add((field_lat_degrees.get_value() >= 0) ? v : -v);
    };

    const auto wrapped_lon_seconds = [this](int32_t v) {
        field_lon_minutes.add(v);
    };

    const auto wrapped_lon_minutes = [this](int32_t v) {
        field_lon_degrees.add((field_lon_degrees.get_value() >= 0) ? v : -v);
    };

    field_lat_seconds.on_wrap = wrapped_lat_seconds;
    field_lat_minutes.on_wrap = wrapped_lat_minutes;
    field_lon_seconds.on_wrap = wrapped_lon_seconds;
    field_lon_minutes.on_wrap = wrapped_lon_minutes;
}

void Locator::set_read_only(bool v) {
    // only setting altitude to read-only (allow manual panning via lat/lon fields)
    // field_altitude.set_focusable(!v);
    // field_speed.set_focusable(!v);
}

// Stupid hack to avoid an event loop
void Locator::set_report_change(bool v) {
    report_change = v;
}

void Locator::on_focus() {
    field_altitude.set_focus(true);
}

void Locator::hide_altandspeed() {

    field_altitude.set_visible(false);
    field_speed.set_visible(false);
}

void Locator::set_altitude(int32_t altitude) {
    field_altitude.set_value(altitude);
}
void Locator::set_speed(int32_t speed) {
    field_speed.set_value(speed);
}

void Locator::set_lat(float lat) {
    field_lat_degrees.set_value(lat);
    field_lat_minutes.set_value((uint32_t)abs(lat / (1.0 / 60)) % 60);
    field_lat_seconds.set_value((uint32_t)abs(lat / (1.0 / 3600)) % 60);
}

void Locator::set_lon(float lon) {
    field_lon_degrees.set_value(lon);
    field_lon_minutes.set_value((uint32_t)abs(lon / (1.0 / 60)) % 60);
    field_lon_seconds.set_value((uint32_t)abs(lon / (1.0 / 3600)) % 60);
}

float Locator::lat() {
    if (field_lat_degrees.get_value() < 0) {
        return -1 * (-1 * field_lat_degrees.get_value() + (field_lat_minutes.get_value() / 60.0) + (field_lat_seconds.get_value() / 3600.0));
    } else {
        return field_lat_degrees.get_value() + (field_lat_minutes.get_value() / 60.0) + (field_lat_seconds.get_value() / 3600.0);
    }

    return 0;
};

float Locator::lon() {
    if (field_lon_degrees.get_value() < 0) {
        return -1 * (-1 * field_lon_degrees.get_value() + (field_lon_minutes.get_value() / 60.0) + (field_lon_seconds.get_value() / 3600.0));
    } else {
        return field_lon_degrees.get_value() + (field_lon_minutes.get_value() / 60.0) + (field_lon_seconds.get_value() / 3600.0);
    }
    return 0;
};

int32_t Locator::altitude() {
    return field_altitude.get_value();
};

int32_t Locator::speed() {
    return field_speed.get_value();
};

} // namespace ui
