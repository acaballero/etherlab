//
// Created by Angel Dust on 23/06/2025.
//

#ifndef __MAP_VIEW_H__
#define __MAP_VIEW_H__

#include "Display_afb.h"
#include "io/fatfs_file.h"
#include "label_widget.h"
#include "field_widget.h"
#include "view.h"
#include <stdint.h>

namespace ui {

#define MAX_MAP_ZOOM_IN 4000
#define MAX_MAP_ZOOM_OUT 10
#define MAP_ZOOM_RESOLUTION_LIMIT 5 // Max zoom-in to show map; rect height & width must divide into this evenly

#define INVALID_LAT_LON 200
#define INVALID_ANGLE 400

#define GEOMAP_BANNER_HEIGHT (3 * 16)
#define GEOMAP_RECT_WIDTH 240
#define GEOMAP_RECT_HEIGHT (320 - 16 - GEOMAP_BANNER_HEIGHT)

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

class Locator : public View {
  public:
    enum alt_unit { FEET = 0, METERS };
    enum spd_unit { NONE = 0, MPH, KMPH, HIDDEN = 255 };

    std::function<void(int32_t, float, float, int32_t)> on_change{};

    Locator(const Point pos, const alt_unit altitude_unit, const spd_unit speed_unit);

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

  private:
    bool read_only{false};
    bool report_change{true};
    alt_unit altitude_unit_{};
    spd_unit speed_unit_{};

    Label labels_position[]{
        {{1 * 8, 0 * 16}, "Alt:", C565_GREY_LIGHT},
        {{1 * 8, 1 * 16}, "Lat:    \xB0  '  \"", Theme::getInstance()->fg_light->foreground}, // 0xB0 is degree ° symbol in our 8x16 font
        {{1 * 8, 2 * 16}, "Lon:    \xB0  '  \"", Theme::getInstance()->fg_light->foreground},
    };
    Labels label_spd_position{
        {{15 * 8, 0 * 16}, "Spd:", Theme::getInstance()->fg_light->foreground},
    };
    Field field_altitude{{6 * 8, 0 * 16}, 5, {-1000, 50000}, " "};

    Field field_speed{{19 * 8, 0 * 16}, 4, {0, 5000}, ' '};
    Label text_alt_unit{{12 * 8, 0 * 16, 2 * 8, 16}};
    Label text_speed_unit{{25 * 8, 0 * 16, 4 * 8, 16}};

    Field field_lat_degrees{{5 * 8, 1 * 16}, 4, {-90, 90}, 1, ' '};
    Field field_lat_minutes{{10 * 8, 1 * 16}, 2, {0, 59}, 1, ' ', true};
    Field field_lat_seconds{{13 * 8, 1 * 16}, 2, {0, 59}, 1, ' ', true};
    Label text_lat_decimal{{17 * 8, 1 * 16, 13 * 8, 1 * 16}};

    Field field_lon_degrees{{5 * 8, 2 * 16}, 4, {-180, 180}, 1, ' '};
    Field field_lon_minutes{{10 * 8, 2 * 16}, 2, {0, 59}, 1, ' ', true};
    Field field_lon_seconds{{13 * 8, 2 * 16}, 2, {0, 59}, 1, ' ', true};
    Label text_lon_decimal{{17 * 8, 2 * 16, 13 * 8, 1 * 16}};
};

enum MarkerStorage { MARKER_NOT_STORED, MARKER_STORED, MARKER_LIST_FULL };

class Map : public Widget {
  public:
    std::function<void(float, float)> on_move{};

    Map(Rect parent_rect);

    void paint_callback() override;

    bool on_input(const st_inputEvent event) override;

    void update_my_position(float lat, float lon, int32_t altitude);
    void update_my_orientation(uint16_t angle, bool refresh = false);

    bool init();
    void set_mode(MapMode mode);
    void set_manual_panning(bool v);
    bool manual_panning();
    void move(const float lon, const float lat);
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
        hide_center_marker_ = hide;
    }
    bool hide_center_marker() {
        return hide_center_marker_;
    }

    static const int NumMarkerListElements = 30;

    void clear_markers();
    MarkerStorage store_marker(Marker &marker);

    static const Dim banner_height = GEOMAP_BANNER_HEIGHT;
    static const Dim geomap_rect_width = GEOMAP_RECT_WIDTH;
    static const Dim geomap_rect_height = GEOMAP_RECT_HEIGHT;

  private:
    void before_paint() override;
    void draw_scale();
    Point item_rect_pixel(Marker &item);
    FloatPoint lat_lon_to_map_pixel(float lat, float lon);
    void draw_marker_item(Marker &item, const Color color, const Color fontColor = C565_WHITE, const Color backColor = C565_BLACK);
    void draw_marker(const FloatPoint itemPoint, const uint16_t itemAngle, const std::string itemTag, const Color color = C565_RED,
                     const Color fontColor = C565_WHITE, const Color backColor = C565_BLACK);
    void draw_markers();
    void draw_mypos();
    void draw_bearing(const FloatPoint origin, const uint16_t angle, uint32_t size, const Color color);
    void draw_map_grid();
    void map_read_line(Color *buffer, uint16_t pixels);

    bool manual_panning_{false};
    bool hide_center_marker_{false};
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

    // the portapack's position data ( for example injected from serial )
    Marker my_pos{INVALID_LAT_LON, INVALID_LAT_LON, INVALID_ANGLE, ""}; // lat, lon, angle, tag
    int32_t my_altitude{0};

    int markerListLen{0};
    Marker markerList[NumMarkerListElements];
    bool redraw_map{false};
};

class MapView : public View {
  public:
    MapView(const std::string &tag, int32_t altitude, Locator::alt_unit altitude_unit, Locator::spd_unit speed_unit, float lat, float lon, uint16_t angle,
            const std::function<void(void)> on_close = nullptr);
    MapView(int32_t altitude, Locator::alt_unit altitude_unit, Locator::spd_unit speed_unit, float lat, float lon,
            const std::function<void(int32_t, float, float, int32_t)> on_done);
    ~MapView();

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

  private:
    void setup();

    const std::function<void(int32_t, float, float, int32_t)> on_done{};
    MapMode mode_{};
    int32_t altitude_{};
    int32_t speed_{};
    Locator::alt_unit altitude_unit_{};
    Locator::spd_unit speed_unit_{};
    float lat_{};
    float lon_{};
    uint16_t angle_{};
    std::function<void(void)> on_close_{nullptr};

    Locator geopos{{0, 0}, altitude_unit_, speed_unit_};

    Map geomap{{0, Map::banner_height, Map::geomap_rect_width, Map::geomap_rect_height}};

    Button button_ok{{DISPLAY_X_PIXELS - 15 * 8, 0, 15 * 8, 1 * 16}, display, "OK"};
};

} /* namespace ui */

#endif
