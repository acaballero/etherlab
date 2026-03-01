//
// Created by Angel Dust on 17/04/2021.
//

#ifndef UI_TYPES_H
#define UI_TYPES_H

#include <stdint.h>
#include <vector>

#include "../../lib/Signal/Signal.h"

#define HEADER_HEIGHT 22
#define STATUS_HEIGHT 22
#define INFO_HEIGHT (DISPLAY_Y_PIXELS - HEADER_HEIGHT * 2 - FFT_WIDGET_HEIGHT - FFT_WATERFALL_HEIGHT - FFT_X_AXIS_HEIGHT)
#define TUNE_INFO_HEIGHT (INFO_HEIGHT / 3)
#define METERS_HEIGHT (2 * INFO_HEIGHT / 3)
#define FFT_INFO_HEIGHT (2 * INFO_HEIGHT / 3)
#define METER_WIDTH (int)((float)DISPLAY_X_PIXELS * (4.0 / 7.0))
#define WATERFALL_TOP (HEADER_HEIGHT + FFT_WIDGET_HEIGHT + FFT_X_AXIS_HEIGHT)
#define MENU_START_Y (WATERFALL_TOP + FFT_WATERFALL_HEIGHT)
//#define MENU_START_Y 0

namespace ui {
enum DIRECTION { LEFT, RIGHT, UP, DOWN };
}

using Coord = int16_t;
using Dim = int16_t;

struct Box {

    int16_t x{0}, y{0};
    uint16_t width{0}, height{0};

    constexpr bool operator==(const Box &other) const {
        return x == other.x && y == other.y && width == other.width && height == other.height;
    }
};

struct Area {

    Box box;
    uint32_t size{0};
    bool show_fps{false};
    float fps{0};
};

struct Point {
  private:
    Coord _x;
    Coord _y;

  public:
    constexpr Point() : _x{0}, _y{0} {
    }

    constexpr Point(int x, int y) : _x{static_cast<Coord>(x)}, _y{static_cast<Coord>(y)} {
    }

    constexpr int x() const {
        return _x;
    }

    constexpr int y() const {
        return _y;
    }

    constexpr Point operator-() const {
        return {-_x, -_y};
    }

    constexpr Point operator+(const Point &p) const {
        return {_x + p._x, _y + p._y};
    }

    constexpr Point operator-(const Point &p) const {
        return {_x - p._x, _y - p._y};
    }

    Point &operator+=(const Point &p) {
        _x += p._x;
        _y += p._y;
        return *this;
    }

    Point &operator-=(const Point &p) {
        _x -= p._x;
        _y -= p._y;
        return *this;
    }

    constexpr bool operator==(const Point &other) const {
        return _x == other._x && _y == other._y;
    }
};

struct Size {
  private:
    Dim _w;
    Dim _h;

  public:
    constexpr Size() : _w{0}, _h{0} {
    }

    constexpr Size(int w, int h) : _w{static_cast<Dim>(w)}, _h{static_cast<Dim>(h)} {
    }

    int width() const {
        return _w;
    }

    int height() const {
        return _h;
    }

    bool is_empty() const {
        return (_w < 1) || (_h < 1);
    }

    constexpr bool operator==(const Size &other) const {
        return _w == other._w && _h == other._h;
    }
};

struct Rect {
  private:
    Point _pos;
    Size _size;

  public:
    constexpr Rect() : _pos{}, _size{} {
    }

    constexpr Rect(int x, int y, int w, int h) : _pos{x, y}, _size{w, h} {
    }

    constexpr Rect(Point pos, Size size) : _pos(pos), _size(size) {
    }

    Point location() const {
        return _pos;
    }

    Size size() const {
        return _size;
    }

    int32_t top() const {
        return _pos.y();
    }

    int32_t bottom() const {
        return _pos.y() + _size.height() - 1;
    }

    int32_t left() const {
        return _pos.x();
    }

    int32_t right() const {
        return _pos.x() + _size.width() - 1;
    }

    int32_t width() const {
        return _size.width();
    }

    int32_t height() const {
        return _size.height();
    }

    void set_height(int h) {
        _size = {_size.width(), h};
    }
    void set_width(int w) {
        _size = {w, _size.height()};
    }

    void set_left(int x) {
        _pos = {x, _pos.y()};
    }
    void set_top(int y) {
        _pos = {_pos.x(), y};
    }

    Point center() const {
        return {_pos.x() + _size.width() / 2, _pos.y() + _size.height() / 2};
    }

    bool is_empty() const {
        return _size.is_empty();
    }

    bool contains(const Point p) const;

    bool contains(const Rect &p) const;

    Rect intersect(const Rect &o) const;

    Rect operator+(const Point &p) const {
        return {_pos + p, _size};
    }

    Rect &operator+=(const Rect &p);

    Rect &operator+=(const Point &p);

    constexpr bool operator==(const Rect &other) const {
        return _pos == other._pos && _size == other._size;
    }

    constexpr bool operator!=(const Rect &other) const {
        return !(*this == other);
    }

    Rect &operator-=(const Point &p);

    std::vector<Rect> operator-(const Rect &r);

    Rect operator-(const Point &p);

    operator bool() const {
        return !_size.is_empty();
    }
};

Area to_area(Rect &r);

int subtract_append(const Rect &a, const Rect &b, std::vector<Rect> &out);

#endif // UI_TYPES_H
