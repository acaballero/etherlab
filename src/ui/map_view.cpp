//
// Created by Angel Dust on 23/06/2025.
//

#include "map_view.h"
#include "Display_afb.h"
#include "input/inputEvent.h"
#include "status.h"

#include <cstring>
#include <stdio.h>

namespace ui {

Locator::Locator(const Point pos, const alt_unit altitude_unit, const spd_unit speed_unit) : View(), altitude_unit_(altitude_unit), speed_unit_(speed_unit) {
    set_parent_rect({pos.x(), pos.y(), DISPLAY_X_PIXELS, 3 * 16});

    add_children({&labels_position, &label_spd_position, &field_altitude, &field_speed, &text_alt_unit, &text_speed_unit, &field_lat_degrees,
                  &field_lat_minutes, &field_lat_seconds, &text_lat_decimal, &field_lon_degrees, &field_lon_minutes, &field_lon_seconds, &text_lon_decimal});

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

        format_double(lat_value, buf, ' ', ' ', 5);
        text_lat_decimal.set_label(buf);
        format_double(lon_value, buf, ' ', ' ', 5);
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
        field_lat_minutes.on_encoder(v);
    };

    const auto wrapped_lat_minutes = [this](int32_t v) {
        field_lat_degrees.on_encoder((field_lat_degrees.value() >= 0) ? v : -v);
    };

    const auto wrapped_lon_seconds = [this](int32_t v) {
        field_lon_minutes.on_encoder(v);
    };

    const auto wrapped_lon_minutes = [this](int32_t v) {
        field_lon_degrees.on_encoder((field_lon_degrees.value() >= 0) ? v : -v);
    };

    field_lat_seconds.on_wrap = wrapped_lat_seconds;
    field_lat_minutes.on_wrap = wrapped_lat_minutes;
    field_lon_seconds.on_wrap = wrapped_lon_seconds;

    field_lon_minutes.on_wrap = wrapped_lon_minutes;

    text_alt_unit.set_label(altitude_unit_ ? "m" : "ft");

    if (speed_unit_ == KMPH) {
        text_speed_unit.set_label("kmph");
    }
    if (speed_unit_ == MPH) {
        text_speed_unit.set_label("mph");
    }
    if (speed_unit_ == HIDDEN) {
        text_speed_unit.hidden(true);
        label_spd_position.hidden(true);
        field_speed.hidden(true);
    }
}

void Locator::set_read_only(bool v) {
    // only setting altitude to read-only (allow manual panning via lat/lon fields)
    field_altitude.set_focusable(!v);
    field_speed.set_focusable(!v);
}

// Stupid hack to avoid an event loop
void Locator::set_report_change(bool v) {
    report_change = v;
}

void Locator::on_focus() {
    if (field_altitude.focusable())
        field_altitude.focus();
    else
        field_lat_degrees.focus();
}

void Locator::hide_altandspeed() {
    // Color altitude grey to indicate it's not updated in manual panning mode
    // field_altitude.set_style(Theme::getInstance()->fg_medium);
    // field_speed.set_style(Theme::getInstance()->fg_medium);
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
    if (field_lat_degrees.value() < 0) {
        return -1 * (-1 * field_lat_degrees.value() + (field_lat_minutes.value() / 60.0) + (field_lat_seconds.value() / 3600.0));
    } else {
        return field_lat_degrees.value() + (field_lat_minutes.value() / 60.0) + (field_lat_seconds.value() / 3600.0);
    }
};

float Locator::lon() {
    if (field_lon_degrees.value() < 0) {
        return -1 * (-1 * field_lon_degrees.value() + (field_lon_minutes.value() / 60.0) + (field_lon_seconds.value() / 3600.0));
    } else {
        return field_lon_degrees.value() + (field_lon_minutes.value() / 60.0) + (field_lon_seconds.value() / 3600.0);
    }
};

int32_t Locator::altitude() {
    return field_altitude.value();
};

int32_t Locator::speed() {
    return field_speed.value();
};

Map::Map(Rect parent_rect) : Widget{parent_rect, &lcd}, markerListLen(0) {
}

bool Map::on_input(const st_inputEvent ev) {

    if (ev.type == INPUT_EVENT_TYPE_ENCODER) {
        int delta = ev.value;
        // Valid map_zoom values are -2 to -MAX_MAP_ZOOM_OUT, and +1 to +MAX_MAP_ZOOM_IN (values of 0 and -1 are not permitted)
        if (delta > 0) {
            if (map_zoom < MAX_MAP_ZOOM_IN) {
                if (map_zoom == -2) {
                    map_zoom = 1;
                } else {
                    // zoom in faster after exceeding the map resolution limit
                    map_zoom += (map_zoom >= MAP_ZOOM_RESOLUTION_LIMIT) ? map_zoom : 1;
                }
            }
        } else if (delta < 0) {
            if (map_zoom > -MAX_MAP_ZOOM_OUT) {
                if (map_zoom == 1) {
                    map_zoom = -2;
                } else {
                    if (map_zoom > MAP_ZOOM_RESOLUTION_LIMIT) {
                        map_zoom /= 2;
                    } else {
                        map_zoom--;
                    }
                }
            }
        } else {
            return false;
        }

        map_visible = map_opened && (map_zoom <= MAP_ZOOM_RESOLUTION_LIMIT);
        zoom_pixel_offset = (map_visible && (map_zoom > 1)) ? (float)map_zoom / 2 : 0.0f;

        // Trigger map redraw
        redraw_map = true;
        set_dirty();
        return true;
    }
    return false;
}

void Map::map_read_line(Color *buffer, uint16_t pixels) {
    if (map_zoom == 1) {
        file.read(buffer, pixels << 1);
    } else if (map_zoom > 1) {
        file.read(buffer, (pixels / map_zoom) << 1);

        // Zoom in: Expand each pixel to "map_zoom" number of pixels.
        // Future TODO:  Add dithering to smooth out the pixelation.
        // As long as MOD(width,map_zoom)==0 then we don't need to check buffer overflow case when stretching last pixel;
        // For 240 width, than means no check is needed for map_zoom values up to 6.
        // (Rectangle height must also divide evenly into map_zoom or we get black lines at end of screen)
        // Note that zooming in results in a map offset of (1/map_zoom) pixels to the right & downward directions (see zoom_pixel_offset).
        for (int i = (geomap_rect_width / map_zoom) - 1; i >= 0; i--) {
            for (int j = 0; j < map_zoom; j++) {
                buffer[(i * map_zoom) + j] = buffer[i];
            }
        }
    } else {
        Color *zoom_out_buffer = new Color[(pixels * (-map_zoom))];
        file.read(zoom_out_buffer, (pixels * (-map_zoom)) << 1);

        // Zoom out:  Collapse each group of "-map_zoom" pixels into one pixel.
        // Future TODO: Average each group of pixels (in both X & Y directions if possible).
        for (int i = 0; i < geomap_rect_width; i++) {
            buffer[i] = zoom_out_buffer[i * (-map_zoom)];
        }
        delete[] zoom_out_buffer;
    }
}

void Map::draw_markers() {
    for (int i = 0; i < markerListLen; ++i) {
        draw_marker_item(markerList[i], C565_BLUE, C565_BLUE, C565_MAGENTA);
    }
}

void Map::draw_marker_item(Marker &item, const Color color, const Color fontColor, const Color backColor) {
    const auto r = screen_rect();
    const Point itemPoint = item_rect_pixel(item);

    if ((itemPoint.x() >= 0) && (itemPoint.x() < r.width()) && (itemPoint.y() > 10) && (itemPoint.y() < r.height())) // Dont draw within symbol size of top
    {
        draw_marker({static_cast<float>(itemPoint.x()), static_cast<float>(itemPoint.y() + r.top())}, item.angle, item.tag, color, fontColor, backColor);
    }
}

// Calculate screen position of item, adjusted for zoom factor.
Point Map::item_rect_pixel(Marker &item) {
    const auto r = screen_rect();
    const auto geomap_rect_half_width = r.width() / 2;
    const auto geomap_rect_half_height = r.height() / 2;

    FloatPoint mapPoint = lat_lon_to_map_pixel(item.lat, item.lon);
    float x = mapPoint.x - x_pos;
    float y = mapPoint.y - y_pos;

    if (map_zoom > 1) {
        x = x * map_zoom + zoom_pixel_offset;
        y = y * map_zoom + zoom_pixel_offset;
    } else if (map_zoom < 0) {
        x = x / (-map_zoom);
        y = y / (-map_zoom);
    }

    x += geomap_rect_half_width;
    y += geomap_rect_half_height;

    return {(int16_t)x, (int16_t)y};
}

// Converts latitude/longitude to pixel coordinates in map file.
// (Note that when map_zoom==1, one pixel in map file corresponds to 1 pixel on screen)
FloatPoint Map::lat_lon_to_map_pixel(float lat, float lon) {
    // Using WGS 84/Pseudo-Mercator projection
    float x = (map_width * (lon + 180) / 360);

    // Latitude calculation based on https://stackoverflow.com/a/10401734/2278659
    double lat_rad = sin(lat * PI / 180);
    float y = (map_height - ((map_world_lon / 2 * log((1 + lat_rad) / (1 - lat_rad))) - map_offset));

    return {x, y};
}

// Draw grid in place of map (when zoom-in level is too high).
void Map::draw_map_grid() {
    const auto r = screen_rect();

    // Grid spacing is just based on zoom at the moment, and centered on screen.
    // TODO: Maybe align with latitude/longitude seconds instead?
    int grid_spacing = map_zoom * 2;
    int x = (r.width() / 2) % grid_spacing;
    int y = (r.height() / 2) % grid_spacing;

    if (map_zoom <= MAP_ZOOM_RESOLUTION_LIMIT) {
        return;
    }

    display->writeRect(0, r.top(), r.width(), r.top() + r.height(), C565_GREY_DARKER);

    for (uint16_t line = y; line < r.height(); line += grid_spacing) {
        //  display->writeRect({{0, r.top() + line}, {r.width(), 1}}, Theme::getInstance()->bg_darker->background);
    }
    for (uint16_t column = x; column < r.width(); column += grid_spacing) {
        //    display.fill_rectangle({{column, r.top()}, {1, r.height()}}, Theme::getInstance()->bg_darker->background);
    }
}

void Map::paint_callback() {
    const auto r = screen_rect();
    std::array<Color, geomap_rect_width> map_line_buffer;
    int16_t zoom_seek_x, zoom_seek_y;

    // Ony redraw map if it moved by at least 1 pixel or the markers list was updated
    if (map_zoom <= 1) {
        // Zooming out, or no zoom
        const int min_diff = abs(map_zoom);
        if ((int)abs(x_pos - prev_x_pos) >= min_diff) {
            redraw_map = true;
        } else if ((int)abs(y_pos - prev_y_pos) >= min_diff) {
            redraw_map = true;
        }
    } else {
        // Zooming in; magnify position differences (utilizing zoom_pixel_offset)
        if ((int)(abs(x_pos - prev_x_pos) * map_zoom) >= 1) {
            redraw_map = true;
        } else if ((int)(abs(y_pos - prev_y_pos) * map_zoom) >= 1) {
            redraw_map = true;
        }
    }

    if (redraw_map) {
        prev_x_pos = x_pos; // Note x_pos/y_pos pixel position in map file now correspond to screen rect CENTER pixel
        prev_y_pos = y_pos;
        redraw_map = false;

        // Adjust starting corner position of map per zoom setting;
        // When zooming in the map should technically by shifted left & up by another map_zoom/2 pixels but
        // the map_read_line() function doesn't handle that yet so we're adjusting markers instead (see zoom_pixel_offset).
        if (map_zoom > 1) {
            zoom_seek_x = x_pos - (float)r.width() / (2 * map_zoom);
            zoom_seek_y = y_pos - (float)r.height() / (2 * map_zoom);
        } else {
            zoom_seek_x = x_pos - (r.width() * abs(map_zoom)) / 2;
            zoom_seek_y = y_pos - (r.height() * abs(map_zoom)) / 2;
        }

        if (map_visible) {
            // Read from map file and display to zoomed scale
            int duplicate_lines = (map_zoom < 0) ? 1 : map_zoom;
            for (uint16_t line = 0; line < (r.height() / duplicate_lines); line++) {
                uint16_t seek_line = zoom_seek_y + ((map_zoom >= 0) ? line : line * (-map_zoom));
                file.seek(4 + ((zoom_seek_x + (map_width * seek_line)) << 1));
                map_read_line(map_line_buffer.data(), r.width());

                for (uint16_t j = 0; j < duplicate_lines; j++) {
                    // display.draw_pixels({0, r.top() + (line * duplicate_lines) + j, r.width(), 1}, map_line_buffer);
                }
            }
        } else {
            // No map data or excessive zoom; just draw a grid
            draw_map_grid();
        }

        // Draw crosshairs in center in manual panning mode
        if (manual_panning_) {
            //  display.fill_rectangle({r.center() - Point(16, 1) + Point(zoom_pixel_offset, zoom_pixel_offset), {32, 2}}, Color::red());
            //   display.fill_rectangle({r.center() - Point(1, 16) + Point(zoom_pixel_offset, zoom_pixel_offset), {2, 32}}, Color::red());
        }

        // Draw the other markers
        draw_markers();
        draw_scale();
        draw_mypos();
    }

    // Draw the marker in the center
    if (!manual_panning_ && !hide_center_marker_) {
        draw_marker(r.center() + Point(zoom_pixel_offset, zoom_pixel_offset), angle, tag, C565_RED, C565_WHITE, C565_BLACK);
    }
}

// bool Map::on_keyboard(KeyboardEvent key) {
//     if (key == '+' || key == ' ')
//         return on_encoder(1);
//     if (key == '-')
//         return on_encoder(-1);

//     return false;
// }

// bool Map::on_touch(const TouchEvent event) {
//     if ((event.type == TouchEvent::Type::Start) && (mode_ == PROMPT)) {
//         set_highlighted(true);
//         if (on_move) {
//             Point p = event.point - screen_rect().center();
//             on_move(p.x() / 2.0 * lon_ratio, p.y() / 2.0 * lat_ratio);
//             return true;
//         }
//     }
//     return false;
// }

void Map::move(const float lon, const float lat) {
    const auto r = screen_rect();

    lon_ = lon;
    lat_ = lat;

    // Calculate x_pos/y_pos in map file corresponding to CENTER pixel of screen rect
    // (Note there is a 1:1 correspondence between map file pixels and screen pixels when map_zoom=1)
    FloatPoint mapPoint = lat_lon_to_map_pixel(lat_, lon_);
    x_pos = mapPoint.x;
    y_pos = mapPoint.y;

    // Cap position
    if (x_pos > (map_width - r.width() / 2)) {
        x_pos = map_width - r.width() / 2;
    }
    if (y_pos > (map_height + r.height() / 2)) {
        y_pos = map_height - r.height() / 2;
    }

    // Scale calculation
    float km_per_deg_lon = cos(lat * PI / 180) * 111.321; // 111.321 km/deg longitude at equator, and 0 km at poles
    pixels_per_km = (r.width() / 2) / km_per_deg_lon;
}

bool Map::init() {
    auto result = file.open("img/world_map.bin");
    map_opened = !result.ok();

    if (map_opened) {
        file.read(&map_width, 2);
        file.read(&map_height, 2);
    } else {
        map_width = 32768;
        map_height = 32768;
    }

    map_visible = map_opened;
    map_center_x = map_width >> 1;
    map_center_y = map_height >> 1;

    lon_ratio = 180.0 / map_center_x;
    lat_ratio = -90.0 / map_center_y;

    map_bottom = sin(-85.05 * PI / 180); // Map bitmap only goes from about -85 to 85 lat
    map_world_lon = map_width / (2 * PI);
    map_offset = (map_world_lon / 2 * log((1 + map_bottom) / (1 - map_bottom)));

    return map_opened;
}

void Map::set_mode(MapMode mode) {
    this->mode = mode;
}

void Map::set_manual_panning(bool v) {
    manual_panning_ = v;
}

bool Map::manual_panning() {
    return manual_panning_;
}

void Map::draw_scale() {
    const auto r = screen_rect();
    uint32_t m = 800000;
    uint32_t scale_width = (map_zoom > 0) ? m * map_zoom * pixels_per_km : m * pixels_per_km / (-map_zoom);
    Color scale_color = (map_visible) ? C565_BLACK : C565_WHITE;
    std::string km_string;

    while (scale_width > (uint32_t)r.width() * (1000 / 2)) {
        scale_width /= 2;
        m /= 2;
    }
    scale_width /= 1000;
    if (m < 1000) {
        //    km_string = to_string_dec_uint(m) + "m";
    } else {
        m += 50; // (add rounding factor for div by 100 below)
        uint32_t km = m / 1000;
        m -= km * 1000;
        if (m == 0) {
            //    km_string = to_string_dec_uint(km) + " km";
        } else {
            //    km_string = to_string_dec_uint(km) + "." + to_string_dec_uint(m / 100, 1) + "km";
        }
    }

    //   display.fill_rectangle({{r.right() - 5 - (uint16_t)scale_width, r.bottom() - 4}, {(uint16_t)scale_width, 2}}, scale_color);
    //  display.fill_rectangle({{r.right() - 5, r.bottom() - 8}, {2, 6}}, scale_color);
    //  display.fill_rectangle({{r.right() - 5 - (uint16_t)scale_width, r.bottom() - 8}, {2, 6}}, scale_color);

    // painter.draw_string({(uint16_t)(r.right() - 25 - scale_width - km_string.length() * 5 / 2), r.bottom() - 10}, ui::font::fixed_5x8, Color::black(),
    //                  Color::white(), km_string);
}

void Map::draw_bearing(const FloatPoint origin, const uint16_t angle, uint32_t size, const Color color) {
    Point arrow_a, arrow_b, arrow_c;

    for (size_t thickness = 0; thickness < 3; thickness++) {
        //  arrow_a = fast_polar_to_point((int)angle, size) + origin;
        //  arrow_b = fast_polar_to_point((int)(angle + 180 - 35), size) + origin;
        //   arrow_c = fast_polar_to_point((int)(angle + 180 + 35), size) + origin;

        //  display.draw_line(arrow_a, arrow_b, color);
        //   display.draw_line(arrow_b, arrow_c, color);
        //   display.draw_line(arrow_c, arrow_a, color);

        size--;
    }

    // display.draw_pixel(origin, color); // 1 pixel indicating center pivot point of bearing symbol
}

void Map::draw_marker(const FloatPoint itemPoint, const uint16_t itemAngle, const std::string itemTag, const Color color, const Color fontColor,
                      const Color backColor) {
    const auto r = screen_rect();

    int tagOffset = 10;
    if (mode == PROMPT) {
        // Cross
        //        display->writeRect(itemPoint - Point(16, 1), 32, 2, color);
        // display.fill_rectangle({itemPoint - Point(1, 16), {2, 32}}, color);
        tagOffset = 16;
    } else if (angle < 360) {
        // if we have a valid angle draw bearing
        draw_bearing(itemPoint, itemAngle, 10, color);
        tagOffset = 10;
    } else {
        // draw a small cross
        // display.fill_rectangle({itemPoint - Point(8, 1), {16, 2}}, color);
        // display.fill_rectangle({itemPoint - Point(1, 8), {2, 16}}, color);
        tagOffset = 8;
    }
    // center tag above point
    if ((itemPoint.y - r.top() >= 32) && (itemTag.find_first_not_of(' ') != itemTag.npos)) { // only draw tag if doesn't overlap top & not just spaces
        // painter.draw_string(itemPoint - Point(((int)itemTag.length() * 8 / 2), 14 + tagOffset), style().font, fontColor, backColor, itemTag);
    }
}

void Map::draw_mypos() {
    if ((my_pos.lat < INVALID_LAT_LON) && (my_pos.lon < INVALID_LAT_LON)) {
        draw_marker_item(my_pos, C565_YELLOW);
    }
}

void Map::clear_markers() {
    markerListLen = 0;
}

MarkerStorage Map::store_marker(Marker &marker) {
    const auto r = screen_rect();
    MarkerStorage ret;

    // Check if it could be on screen
    // (Shows more distant planes when zoomed out)
    FloatPoint mapPoint = lat_lon_to_map_pixel(marker.lat, marker.lon);
    int x_dist = abs((int)mapPoint.x - (int)x_pos);
    int y_dist = abs((int)mapPoint.y - (int)y_pos);
    int zoom_out = (map_zoom < 0) ? -map_zoom : 1;

    if ((x_dist >= (zoom_out * r.width() / 2)) || (y_dist >= (zoom_out * r.height() / 2))) {
        ret = MARKER_NOT_STORED;
    } else if (markerListLen < NumMarkerListElements) {
        markerList[markerListLen] = marker;
        markerListLen++;
        redraw_map = true;
        ret = MARKER_STORED;
    } else {
        ret = MARKER_LIST_FULL;
    }
    return ret;
}

void Map::update_my_position(float lat, float lon, int32_t altitude) {
    my_pos.lat = lat;
    my_pos.lon = lon;
    my_altitude = altitude;
    redraw_map = true;
    set_dirty();
}

void Map::update_my_orientation(uint16_t angle, bool refresh) {
    my_pos.angle = angle;
    if (refresh) {
        redraw_map = true;
        set_dirty();
    }
}

void MapView::on_focus() {
    geopos.focus();

    if (!geomap.map_file_opened()) {
        status::handleError(status::ST_ERROR, "No world_map.bin file");
    }
}

void MapView::update_my_position(float lat, float lon, int32_t altitude) {
    geomap.update_my_position(lat, lon, altitude);
}
void MapView::update_my_orientation(uint16_t angle, bool refresh) {
    geomap.update_my_orientation(angle, refresh);
}

void MapView::update_position(float lat, float lon, uint16_t angle, int32_t altitude, int32_t speed) {
    if (geomap.manual_panning()) {
        geomap.set_dirty();
        return;
    }

    lat_ = lat;
    lon_ = lon;
    altitude_ = altitude;
    speed_ = speed;

    // Stupid hack to avoid an event loop
    geopos.set_report_change(false);
    geopos.set_lat(lat_);
    geopos.set_lon(lon_);
    geopos.set_altitude(altitude_);
    geopos.set_speed(speed_);
    geopos.set_report_change(true);

    geomap.set_angle(angle);
    geomap.move(lon_, lat_);
    geomap.set_dirty();
}

void MapView::update_tag(const std::string tag) {
    geomap.set_tag(tag);
}

void MapView::setup() {
    add_child(&geomap);

    geopos.set_altitude(altitude_);
    geopos.set_lat(lat_);
    geopos.set_lon(lon_);

    geopos.on_change = [this](int32_t altitude, float lat, float lon, int32_t speed) {
        altitude_ = altitude;
        lat_ = lat;
        lon_ = lon;
        speed_ = speed;
        geopos.hide_altandspeed();
        geomap.set_manual_panning(true);
        geomap.move(lon_, lat_);
        geomap.set_dirty();
    };

    geomap.on_move = [this](float move_x, float move_y) {
        lon_ += move_x;
        lat_ += move_y;

        // Stupid hack to avoid an event loop
        geopos.set_report_change(false);
        geopos.set_lon(lon_);
        geopos.set_lat(lat_);
        geopos.set_report_change(true);

        geomap.move(lon_, lat_);
        geomap.set_dirty();
    };
}

MapView::~MapView() {
    if (on_close_) {
        on_close_();
    }
}

// Display mode
MapView::MapView(const std::string &tag, int32_t altitude, Locator::alt_unit altitude_unit, Locator::spd_unit speed_unit, float lat, float lon, uint16_t angle,
                 const std::function<void(void)> on_close)
    : View(), altitude_(altitude), altitude_unit_(altitude_unit), speed_unit_(speed_unit), lat_(lat), lon_(lon), angle_(angle), on_close_(on_close) {
    mode_ = DISPLAY;

    add_child(&geopos);

    geomap.init();

    setup();

    geomap.set_mode(mode_);
    geomap.set_tag(tag);
    geomap.set_angle(angle);
    geomap.move(lon_, lat_);

    geopos.set_read_only(true);
}

// Prompt mode
MapView::MapView(int32_t altitude, Locator::alt_unit altitude_unit, Locator::spd_unit speed_unit, float lat, float lon,
                 const std::function<void(int32_t, float, float, int32_t)> on_done)
    : View(), altitude_(altitude), altitude_unit_(altitude_unit), speed_unit_(speed_unit), lat_(lat), lon_(lon) {
    mode_ = PROMPT;

    add_child(&geopos);

    geomap.init();

    setup();
    add_child(&button_ok);

    geomap.set_mode(mode_);
    geomap.move(lon_, lat_);

    button_ok.action = [this, on_done](Button &, st_inputEvent) {
        if (on_done) {
            on_done(altitude_, lat_, lon_, speed_);
        }
        this->set_visible(false);
    };
}

void MapView::clear_markers() {
    geomap.clear_markers();
}

MarkerStorage MapView::store_marker(Marker &marker) {
    return geomap.store_marker(marker);
}

} /* namespace ui */
