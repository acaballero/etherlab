//
// Created by Angel Dust on 23/06/2025.
//

#ifndef __MAP_VIEW_H__
#define __MAP_VIEW_H__

#include "Display_afb.h"
#include "io/fatfs_file.h"
#include "ips_font.h"
#include "label_widget.h"
#include "field_widget.h"
#include "menu_options.h"
#include "locator_view.h"
#include "number_field_widget.h"
#include "ui/ui_types.h"
#include "view.h"
#include <stdint.h>
#include <type_traits>

namespace ui {

#define MAX_MAP_ZOOM_IN 4000
#define MAX_MAP_ZOOM_OUT 10
#define MAP_ZOOM_RESOLUTION_LIMIT 3 // Max zoom-in to show map; rect height & width must divide into this evenly

#define INVALID_LAT_LON 200
#define INVALID_ANGLE 400

#define MAPVIEW_Y_POS HEADER_HEIGHT
#define MAP_TITLE_HEIGHT HEADER_HEIGHT
#define MAPVIEW_WIDTH DISPLAY_X_PIXELS
#define MAPVIEW_HEIGHT (DISPLAY_Y_PIXELS - MAPVIEW_Y_POS - STATUS_HEIGHT)

enum MapMode { DISPLAY, PROMPT };

struct FloatPoint {
  public:
    float x{0};
    float y{0};
};

struct Marker {
  public:
    float lat{0};
    float lon{0};
    uint16_t angle{0};
    std::string tag{""};

    Marker &operator=(Marker &rhs) {
        lat = rhs.lat;
        lon = rhs.lon;
        angle = rhs.angle;
        tag = rhs.tag;

        return *this;
    }
};

enum MarkerStorage { MARKER_NOT_STORED, MARKER_STORED, MARKER_LIST_FULL };

class Map : public Widget {
  public:
    std::function<void(float, float)> on_move{};

    Map(Rect parent_rect);

    bool paint_callback() override;

    bool on_input(const st_inputEvent event) override;

    void update_my_position(float lat, float lon, int32_t altitude);
    void update_my_orientation(uint16_t angle, bool refresh = false);

    bool init();
    void set_mode(MapMode mode);
    void set_manual_panning(bool v);
    bool get_manual_panning();
    void move(const float lon, const float lat);
    void pan(const int dx, const int dy);
    void set_tag(std::string new_tag) {
        tag = new_tag;
    }

    void set_angle(uint16_t new_angle) {
        angle = new_angle;
    }

    bool map_file_opened() {
        return map_opened;
    }

    void set_hide_center_marker(bool hide) {
        hide_center_marker = hide;
    }
    bool get_hide_center_marker() {
        return hide_center_marker;
    }

    static const int NumMarkerListElements = 30;

    void clear_markers();
    MarkerStorage store_marker(Marker &marker);

    static const Dim map_top = MAP_TITLE_HEIGHT;
    static const Dim map_rect_width = MAPVIEW_WIDTH;
    static const Dim map_rect_height = MAPVIEW_HEIGHT - map_top;

  private:
    void before_paint() override;
    void draw_scale();
    Point item_rect_pixel(Marker &item);
    FloatPoint lat_lon_to_map_pixel(float lat, float lon);
    void draw_marker_item(Marker &item, const Color color, const Color fontColor = C565_WHITE, const Color backColor = C565_BLACK);
    void draw_marker(const Point itemPoint, const uint16_t itemAngle, const std::string itemTag, const Color color = C565_RED,
                     const Color fontColor = C565_WHITE, const Color backColor = C565_BLACK);
    void draw_markers();
    void draw_mypos();
    void draw_bearing(const Point origin, const uint16_t angle, uint32_t size, const Color color);
    void draw_map_grid();
    Point polar_to_point(int32_t angle, uint32_t distance);
    void map_read_line(Color *buffer, uint16_t pixels);

    bool manual_panning{true};
    bool hide_center_marker{true};
    MapMode mode{};
    FatFSFile file{};
    bool map_opened{};
    bool map_visible{};
    uint16_t map_width{}, map_height{};
    int32_t map_center_x{}, map_center_y{};
    int16_t map_zoom{1};
    float lon_ratio{}, lat_ratio{};
    double map_bottom{};
    double map_world_lon{};
    double map_offset{};

    float x_pos{}, y_pos{};
    float prev_x_pos{32767.0f}, prev_y_pos{32767.0f};
    float lat_{};
    float lon_{};
    float zoom_pixel_offset{0.0f};
    float pixels_per_km{};
    uint16_t angle{};
    std::string tag{};

    Marker my_pos{INVALID_LAT_LON, INVALID_LAT_LON, INVALID_ANGLE, ""}; // lat, lon, angle, tag
    int32_t my_altitude{0};

    int markerListLen{0};
    Marker markerList[NumMarkerListElements];
};

class MapView : public View {
  public:
    MapView(const std::string &tag, int32_t altitude, Locator::alt_unit altitude_unit, Locator::spd_unit speed_unit, float lat, float lon, uint16_t angle,
            const std::function<void(void)> on_close = nullptr);
    MapView(int32_t altitude, Locator::alt_unit altitude_unit, Locator::spd_unit speed_unit, float lat, float lon,
            const std::function<void(int32_t, float, float, int32_t)> on_done);

    MapView(const MapView &) = delete;
    MapView(MapView &&) = delete;
    MapView &operator=(const MapView &) = delete;
    MapView &operator=(MapView &&) = delete;

    void on_focus() override;

    void update_position(float lat, float lon, uint16_t angle, int32_t altitude, int32_t speed = 0);
    void update_my_position(float lat, float lon, int32_t altitude);
    void update_my_orientation(uint16_t angle, bool refresh = false);

    void clear_markers();
    MarkerStorage store_marker(Marker &marker);

    void update_tag(const std::string tag);

    bool on_input(const st_inputEvent e) override;

    void exit();

  private:
    void before_paint() override{};

    void setup();

    const std::function<void(int32_t, float, float, int32_t)> on_done{};
    MapMode mode{};
    int32_t altitude{-1};
    int32_t speed{-1};
    Locator::alt_unit altitude_unit{};
    Locator::spd_unit speed_unit{};
    float lat{};
    float lon{};
    uint16_t angle{};
    std::function<void(void)> on_close{nullptr};

    Locator locator{{2, 10}, altitude_unit, speed_unit};

    Map map{{0, Map::map_top, Map::map_rect_width, Map::map_rect_height}};

    Menu::menu_action_st menu_actions[5] = {{"<",
                                             [this]() {
                                                 map.pan(40, 0);
                                             }},
                                            {">",
                                             [this]() {
                                                 map.pan(-40, 0);
                                             }},
                                            {"up",
                                             [this]() {
                                                 map.pan(0, -40);
                                             }},
                                            {"down",
                                             [this]() {
                                                 map.pan(0, 40);
                                             }},
                                            {"Exit", [this]() {
                                                 exit();
                                             }}};

    Menu::menu_actions_st actions = {menu_actions, sizeof(menu_actions) / sizeof(Menu::menu_action_st)};

    Menu::menu_actions_st *get_quick_actions() override {
        return &actions;
    }
};

} /* namespace ui */

#endif
