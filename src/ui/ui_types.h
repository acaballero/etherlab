//
// Created by Angel Dust on 17/04/2021.
//

#ifndef UI_TYPES_H
#define UI_TYPES_H

//
// Created by Angel Dust on 17/04/2021.
//

#include <stdint.h>

using Coord = int16_t;
using Dim = int16_t;

struct Point {
  private:
    Coord _x;
    Coord _y;

  public:
    constexpr Point() : _x{0}, _y{0} {}

    constexpr Point(int x, int y) : _x{static_cast<Coord>(x)}, _y{static_cast<Coord>(y)} {}

    constexpr int x() const { return _x; }

    constexpr int y() const { return _y; }

    constexpr Point operator-() const { return {-_x, -_y}; }

    constexpr Point operator+(const Point &p) const { return {_x + p._x, _y + p._y}; }

    constexpr Point operator-(const Point &p) const { return {_x - p._x, _y - p._y}; }

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
};

struct Size {
  private:
    Dim _w;
    Dim _h;

  public:
    constexpr Size() : _w{0}, _h{0} {}

    constexpr Size(int w, int h) : _w{static_cast<Dim>(w)}, _h{static_cast<Dim>(h)} {}

    int width() const { return _w; }

    int height() const { return _h; }

    bool is_empty() const { return (_w < 1) || (_h < 1); }
};

struct Rect {
  private:
    Point _pos;
    Size _size;

  public:
    constexpr Rect() : _pos{}, _size{} {}

    constexpr Rect(int x, int y, int w, int h) : _pos{x, y}, _size{w, h} {}

    constexpr Rect(Point pos, Size size) : _pos(pos), _size(size) {}

    Point location() const { return _pos; }

    Size size() const { return _size; }

    int top() const { return _pos.y(); }

    int bottom() const { return _pos.y() + _size.height() - 1; }

    int left() const { return _pos.x(); }

    int right() const { return _pos.x() + _size.width() - 1; }

    int width() const { return _size.width(); }

    int height() const { return _size.height(); }

    Point center() const { return {_pos.x() + _size.width() / 2, _pos.y() + _size.height() / 2}; }

    bool is_empty() const { return _size.is_empty(); }

    bool contains(const Point p) const;

    Rect intersect(const Rect &o) const;

    Rect operator+(const Point &p) const { return {_pos + p, _size}; }

    Rect &operator+=(const Rect &p);

    Rect &operator+=(const Point &p);

    Rect &operator-=(const Point &p);

    operator bool() const { return !_size.is_empty(); }
};

#endif // UI_TYPES_H
