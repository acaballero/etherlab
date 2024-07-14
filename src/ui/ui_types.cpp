//
// Created by Angel Dust on 17/04/2021.
//

#include "ui_types.h"
#include <stdio.h>
#include "../../lib/utils/utils.hpp"


bool Rect::contains(const Point p) const {
    return (p.x() >= left()) && (p.y() >= top()) &&
           (p.x() < right()) && (p.y() < bottom());
}

Rect Rect::intersect(const Rect &o) const {
    const auto x1 = max2(left(), o.left());
    const auto x2 = min2(right(), o.right());
    const auto y1 = max2(top(), o.top());
    const auto y2 = min2(bottom(), o.bottom());
    if ((x2 >= x1) && (y2 > y1)) {
        return {x1, y1, x2 - x1, y2 - y1};
    } else {
        return {};
    }
}

// TODO: This violates the principle of least surprise!
// This does a union, but that might not be obvious from "+=" syntax.
Rect &Rect::operator+=(const Rect &p) {
    if (is_empty()) {
        *this = p;
    }
    if (!p.is_empty()) {
        const auto x1 = min2(left(), p.left());
        const auto y1 = min2(top(), p.top());
        _pos = {x1, y1};
        const auto x2 = max2(right(), p.right());
        const auto y2 = max2(bottom(), p.bottom());
        _size = {x2 - x1, y2 - y1};
    }
    return *this;
}

Rect &Rect::operator+=(const Point &p) {
    _pos += p;
    return *this;
}

Rect &Rect::operator-=(const Point &p) {
    _pos -= p;
    return *this;
}