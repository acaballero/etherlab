//
// Created by Angel Dust on 23/06/2025.
//

#include "map_view.h"
#include "Display_afb.h"
#include "fatfs/fatfs.h"
#include "ff.h"
#include "input/inputEvent.h"
#include "io/fatfs_file.h"
#include "ips_font.h"
#include "status.h"
#include "utils.hpp"

#include <cstddef>
#include <cstring>
#include <stdio.h>
#include <stm32f4xx.h>

namespace ui {

Locator::Locator(const Point pos, const alt_unit altitude_unit, const spd_unit speed_unit, FontDef *f)
    : View(), altitude_unit_(altitude_unit), speed_unit_(speed_unit) {

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
        for (int i = (map_rect_width / map_zoom) - 1; i >= 0; i--) {
            for (int j = 0; j < map_zoom; j++) {
                buffer[(i * map_zoom) + j] = buffer[i];
            }
        }
    } else {
        Color *zoom_out_buffer = new Color[(pixels * (-map_zoom))];
        if (zoom_out_buffer) {
            file.read(zoom_out_buffer, (pixels * (-map_zoom)) << 1);

            // Zoom out:  Collapse each group of "-map_zoom" pixels into one pixel.
            // TODO: Use mean value of adjacent pixels.
            for (int i = 0; i < map_rect_width; i++) {
                buffer[i] = zoom_out_buffer[i * (-map_zoom)];
            }
            delete[] zoom_out_buffer;
        } else {
            status::pop_alert(status::ERROR, "MapView: Can't allocate line buffer");
        }
    }
}

void Map::draw_markers() {
    for (int i = 0; i < markerListLen; ++i) {
        draw_marker_item(markerList[i], C565_BLUE, C565_BLUE, C565_MAGENTA);
    }
}

void Map::draw_marker_item(Marker &item, const Color color, const Color fontColor, const Color backColor) {
    const auto r = parent_rect();
    const Point itemPoint = item_rect_pixel(item);

    if ((itemPoint.x() >= 0) && (itemPoint.x() < r.width()) && (itemPoint.y() > 10) && (itemPoint.y() < r.height())) // Dont draw within symbol size of top
    {
        draw_marker({itemPoint.x(), itemPoint.y() + r.top()}, item.angle, item.tag, color, fontColor, backColor);
    }
}

// Calculate screen position of item, adjusted for zoom factor.
Point Map::item_rect_pixel(Marker &item) {
    const auto r = parent_rect();
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

void Map::draw_map_grid() {
    const auto r = parent_rect();

    // Grid spacing is just based on zoom at the moment, and centered on screen.
    // TODO: Maybe align with latitude/longitude seconds instead?
    int grid_spacing = map_zoom * 2;
    int x = (r.width() / 2) % grid_spacing;
    int y = (r.height() / 2) % grid_spacing;

    if (map_zoom <= MAP_ZOOM_RESOLUTION_LIMIT) {
        return;
    }

    for (uint16_t line = y; line < r.height(); line += grid_spacing) {
        display->writeRect({0, static_cast<uint32_t>(line)}, {static_cast<uint32_t>(r.width()), 1}, C565_GREY_DARKER);
    }
    for (uint16_t column = x; column < r.width(); column += grid_spacing) {
        display->writeRect({column, 0}, {1, static_cast<uint32_t>(r.height())}, C565_GREY_DARKER);
    }
}

bool Map::paint_callback() {
    const auto r = parent_rect();
    std::array<Color, map_rect_width> map_line_buffer;
    int16_t zoom_seek_x, zoom_seek_y;

    display->setFont((FontDef *)&Font_Tiny8x8);

    prev_x_pos = x_pos; // Note x_pos/y_pos pixel position in map file now correspond to screen rect CENTER pixel
    prev_y_pos = y_pos;

    // Adjust starting corner position of map per zoom setting;
    // When zooming in the map should technically by shifted left & up by another map_zoom/2 pixels but
    // the map_locaread_line() function doesn't handle that yet so we're adjusting markers instead (see zoom_pixel_offset).
    if (map_zoom > 1) {
        zoom_seek_x = x_pos - (float)r.width() / (2 * map_zoom);
        zoom_seek_y = y_pos - (float)r.height() / (2 * map_zoom);
    } else {
        zoom_seek_x = x_pos - (r.width() * abs(map_zoom)) / 2;
        zoom_seek_y = y_pos - (r.height() * abs(map_zoom)) / 2;
    }

    if (map_visible) {
        int16_t oy = display->get_offset().y;
        int16_t y1 = display->current_line - oy;

        // Read from map file and disqqplay to zoomed scale
        int duplicate_lines = (map_zoom < 0) ? 1 : map_zoom;
        int nlines = display->chunk_height / duplicate_lines;
        for (uint16_t line = 0; line < nlines; line++) {

            int widget_line = line + y1;
            volatile int seek_line = zoom_seek_y + ((map_zoom >= 0) ? widget_line : (widget_line * (-map_zoom)));
            if (seek_line >= 0) {
                auto offset = 4 + ((zoom_seek_x + (map_width * seek_line)) << 1);
                if (file.seek(offset) == FR_OK) { // skip 4 bytes
                    map_read_line(map_line_buffer.data(), r.width());

                    for (uint16_t j = 0; j < duplicate_lines; j++) {
                        for (uint16_t x = 0; x < r.width(); x++) {
                            display->setPixel(x, y1 + (line * duplicate_lines) + j, SWAP_BYTES(map_line_buffer[x]));
                        }
                    }
                } else {
                    return false;
                }
            }
        }
    } else {

        // No map data or excessive zoom; just draw a grid
        display->clear();
        draw_map_grid();
    }

    // Draw crosshairs in center in manual panning mode
    if (manual_panning) {
        Point p1 = r.center() - Point(16, 1) + Point(zoom_pixel_offset, zoom_pixel_offset);
        Point p2 = r.center() - Point(1, 16) + Point(zoom_pixel_offset, zoom_pixel_offset);
        display->writeRect(p1.x(), p1.y(), p1.x() + 32, p1.y() + 2, C565_RED);
        display->writeRect(p2.x(), p2.y(), p2.x() + 2, p2.y() + 32, C565_RED);
    }

    // Draw the other markers
    draw_markers();
    draw_scale();
    draw_mypos();

    // Draw the marker in the center
    if (!manual_panning && !hide_center_marker) {
        draw_marker(r.center() + Point(zoom_pixel_offset, zoom_pixel_offset), angle, tag, C565_RED, C565_WHITE, C565_BLACK);
    }

    return true;
}

void Map::pan(const int dx, const int dy) {
    float factor = map_zoom > 0 ? (1.0f / map_zoom) : (-map_zoom);
    on_move(dx * factor * lon_ratio, dy * factor * lat_ratio);
}

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
    const auto r = parent_rect();

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
    map_opened = result.ok();

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
    manual_panning = v;
}

bool Map::get_manual_panning() {
    return manual_panning;
}

void Map::draw_scale() {
    const auto r = parent_rect();
    uint32_t m = 800000;
    uint32_t margin = 7;
    uint32_t scale_width = (map_zoom > 0) ? m * map_zoom * pixels_per_km : m * pixels_per_km / (-map_zoom);
    Color scale_color = (map_visible) ? C565_BLACK : C565_WHITE;
    char buf[10], units[3];

    while (scale_width > (uint32_t)r.width() * (1000 / 2)) {
        scale_width /= 2;
        m /= 2;
    }
    scale_width /= 1000;
    format_eng(buf, m, "m", units);
    sprintf(buf + strlen(buf), "%s", units);

    display->fill({r.width() - 5 - scale_width, static_cast<uint32_t>(r.height() - margin)}, {(uint16_t)scale_width, 3}, scale_color);
    display->fill({static_cast<uint32_t>(r.width() - 5), static_cast<uint32_t>(r.height() - margin - 4)}, {2, 6}, scale_color);
    display->fill({r.width() - 5 - scale_width, static_cast<uint32_t>(r.height() - margin - 4)}, {2, 6}, scale_color);

    uint32_t label_w = strlen(buf) * font->width;
    uint32_t label_x = r.width() - 15 - scale_width - label_w;
    display->gotoXY(label_x, r.height() - margin - 6);
    display->fill({label_x - 4, static_cast<uint32_t>(r.height() - margin - 8)}, {label_w + 8, static_cast<uint32_t>(font->height + 5)}, scale_color);
    display->setColor(map_visible ? C565_WHITE : C565_BLACK);
    display->setBgColor(C565_TRANSPARENT);
    display->write(buf);
}

Point Map::polar_to_point(int32_t angle, uint32_t distance) {
    // polar to compass with y negated for screen drawing
    return Point((int16_sin_s4(((1 << 16) * (-angle + 180)) / 360) * distance) / (1 << 16),
                 (int16_sin_s4(((1 << 16) * (-angle - 90)) / 360) * distance) / (1 << 16));
}

void Map::draw_bearing(const Point origin, const uint16_t angle, uint32_t size, const Color color) {
    Point arrow_a, arrow_b, arrow_c;

    for (size_t thickness = 0; thickness < 3; thickness++) {
        arrow_a = polar_to_point((int)angle, size) + origin;
        arrow_b = polar_to_point((int)(angle + 180 - 35), size) + origin;
        arrow_c = polar_to_point((int)(angle + 180 + 35), size) + origin;

        display->writeLine(arrow_a.x(), arrow_a.y(), arrow_b.x(), arrow_b.y(), color);
        display->writeLine(arrow_b.x(), arrow_b.y(), arrow_c.x(), arrow_c.y(), color);
        display->writeLine(arrow_c.x(), arrow_c.y(), arrow_a.x(), arrow_a.y(), color);

        size--;
    }

    display->setPixel(origin.x(), origin.y(), color); // 1 pixel indicating center pivot point of bearing symbol
}

void Map::draw_marker(const Point item_point, const uint16_t item_angle, const std::string item_tag, const Color color, const Color fg_color,
                      const Color bg_color) {
    const auto r = parent_rect();

    int tagOffset = 10;
    if (mode == PROMPT) {
        // Cross
        Point p1 = item_point - Point(16, 1);
        Point p2 = item_point - Point(1, 16);

        display->writeRect({static_cast<uint32_t>(p1.x()), static_cast<uint32_t>(p2.y())}, {32, 2}, color);
        display->fill({static_cast<uint32_t>(p2.x()), static_cast<uint32_t>(p2.y())}, {2, 32}, color);
        tagOffset = 16;
    } else if (angle < 360) {
        // if we have a valid angle draw bearing
        draw_bearing(item_point, item_angle, 10, color);
        tagOffset = 10;
    } else {
        Point p1 = item_point - Point(8, 1);
        Point p2 = item_point - Point(1, 8);
        // Cross
        display->writeRect({static_cast<uint32_t>(p1.x()), static_cast<uint32_t>(p2.y())}, {32, 2}, color);
        display->writeRect({static_cast<uint32_t>(p2.x()), static_cast<uint32_t>(p2.y())}, {2, 32}, color);
        tagOffset = 8;
    }
    // center tag above point
    if ((item_point.y() - r.top() >= 32) && (item_tag.find_first_not_of(' ') != item_tag.npos)) { // only draw tag if doesn't overlap top & not just spaces
        Point p1 = item_point - Point(((int)item_tag.length() * 8 / 2), 14 + tagOffset);
        display->gotoXY(p1.x(), p1.y());
        display->setColor(fg_color);
        display->setBgColor(bg_color);
        display->write(item_tag.c_str());
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
    const auto r = parent_rect();
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
        set_dirty();
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

    set_dirty();
}

void Map::update_my_orientation(uint16_t angle, bool refresh) {
    my_pos.angle = angle;
    if (refresh) {

        set_dirty();
    }
}

void Map::before_paint() {
    bool dirty = false;
    // Ony redraw map if it moved by at least 1 pixel or the markers list was updated
    if (map_zoom <= 1) {
        // Zooming out, or no zoom
        const int min_diff = abs(map_zoom);
        if ((int)abs(x_pos - prev_x_pos) >= min_diff) {
            dirty = true;
        } else if ((int)abs(y_pos - prev_y_pos) >= min_diff) {
            dirty = true;
        }
    } else {
        // Zooming in; magnify position differences (utilizing zoom_pixel_offset)
        if ((int)(abs(x_pos - prev_x_pos) * map_zoom) >= 1) {
            dirty = true;
        } else if ((int)(abs(y_pos - prev_y_pos) * map_zoom) >= 1) {
            dirty = true;
        }
    }

    if (this->dirty() || dirty) {
        set_dirty();
        display->set_use_dma(false); // Note: Only for the next redraw
    }
}

void MapView::on_focus() {
    locator.set_focus(true);

    if (!map.map_file_opened()) {
        status::pop_alert(status::ERROR, "No world_map.bin file");
    }
}

void MapView::update_my_position(float lat, float lon, int32_t altitude) {
    map.update_my_position(lat, lon, altitude);
}
void MapView::update_my_orientation(uint16_t angle, bool refresh) {
    map.update_my_orientation(angle, refresh);
}

void MapView::update_position(float lat, float lon, uint16_t angle, int32_t altitude, int32_t speed) {
    if (map.get_manual_panning()) {
        map.set_dirty();
        return;
    }

    this->lat = lat;
    this->lon = lon;
    this->altitude = altitude;
    this->speed = speed;

    // Stupid hack to avoid an event loop
    locator.set_report_change(false);
    locator.set_lat(lat);
    locator.set_lon(lon);
    locator.set_altitude(altitude);
    locator.set_speed(speed);
    locator.set_report_change(true);

    map.set_angle(angle);
    map.move(lon, lat);
    map.set_dirty();
}

void MapView::update_tag(const std::string tag) {
    map.set_tag(tag);
}

void MapView::setup() {
    add_child(&map);
    map.set_focus(true);

    if (altitude < 0 && speed < 0) {
        locator.hide_altandspeed();
    } else {
        locator.set_altitude(altitude);
    }

    locator.set_lat(lat);
    locator.set_lon(lon);

    locator.set_z_index(100);

    locator.on_change = [this](int32_t altitude, float lat, float lon, int32_t speed) {
        this->altitude = altitude;
        this->lat = lat;
        this->lon = lon;
        this->speed = speed;
        locator.hide_altandspeed();
        map.set_manual_panning(true);
        map.move(lon, lat);
        map.set_dirty();
    };

    map.on_move = [this](float move_x, float move_y) {
        lon += move_x;
        lat += move_y;

        // Stupid hack to avoid an event loop
        locator.set_report_change(false);
        locator.set_lon(lon);
        locator.set_lat(lat);
        locator.set_report_change(true);

        map.move(lon, lat);
        map.set_dirty();
    };

    actions_signal.emit(&actions);
}

// Display mode
MapView::MapView(const std::string &tag, int32_t altitude, Locator::alt_unit altitude_unit, Locator::spd_unit speed_unit, float lat, float lon, uint16_t angle,
                 const std::function<void(void)> on_close)
    : View({0, MAPVIEW_Y_POS, MAPVIEW_WIDTH, MAPVIEW_HEIGHT}), altitude(altitude), altitude_unit(altitude_unit), speed_unit(speed_unit), lat(lat), lon(lon),
      angle(angle), on_close(on_close) {
    mode = DISPLAY;

    add_child(&locator);

    map.init();

    setup();

    map.set_mode(mode);
    map.set_tag(tag);
    map.set_angle(angle);
    map.move(lon, lat);

    locator.set_read_only(true);
}

// Prompt mode
MapView::MapView(int32_t altitude, Locator::alt_unit altitude_unit, Locator::spd_unit speed_unit, float lat, float lon,
                 const std::function<void(int32_t, float, float, int32_t)> on_done)
    : View({0, MAPVIEW_Y_POS, MAPVIEW_WIDTH, MAPVIEW_HEIGHT}), altitude(altitude), altitude_unit(altitude_unit), speed_unit(speed_unit), lat(lat), lon(lon) {
    mode = PROMPT;

    add_child(&locator);

    map.init();

    setup();

    map.set_mode(mode);
    map.move(lon, lat);
}

void MapView::exit() {
    actions_signal.emit(nullptr);
    set_visible(false);

    if (on_close) {
        on_close();
    }

    if (on_done) {
        on_done(this->altitude, this->lat, this->lon, this->speed);
    }
}

bool MapView::on_input(const st_inputEvent e) {
    // TODO: Too much code just to close the view

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

void MapView::clear_markers() {
    map.clear_markers();
}

MarkerStorage MapView::store_marker(Marker &marker) {
    return map.store_marker(marker);
}

} /* namespace ui */
