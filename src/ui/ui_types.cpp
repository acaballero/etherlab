//
// Created by Angel Dust on 17/04/2021.
//

#include "ui_types.h"
#include <stdio.h>
#include <algorithm>
#include "../../lib/utils/utils.hpp"

bool Rect::contains(const Point p) const { return (p.x() >= left()) && (p.y() >= top()) && (p.x() < right()) && (p.y() < bottom()); }

Rect Rect::intersect(const Rect &o) const {
    const auto x1 = max2(left(), o.left());
    const auto x2 = min2(right(), o.right());
    const auto y1 = max2(top(), o.top());
    const auto y2 = min2(bottom(), o.bottom());
    if ((x2 > x1) && (y2 > y1)) { // consider border as overlapping? not for now
        return {x1, y1, x2 - x1 + 1, y2 - y1 + 1};
    } else {
        return {};
    }
}

bool Rect::contains(const Rect &o) const { return left() <= o.left() && right() >= o.right() && top() <= o.top() && bottom() >= o.bottom(); }

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
        _size = {x2 - x1 + 1, y2 - y1 + 1};
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

Rect Rect::operator-(const Point &p) {
    Rect r = *this;
    r -= p;
    return r;
}

std::vector<Rect> Rect::operator-(const Rect &r) {

    std::vector<Rect> result;

    //  No overlap
    if (this->left() >= r.right() || this->right() <= r.left() || this->top() >= r.bottom() || this->bottom() <= r.top()) {
        result.push_back(*this); // No change
        return result;
    }

    //  Full overlap
    if (this->left() <= r.left() && this->top() <= r.top() && this->right() >= r.right() && this->bottom() >= r.bottom()) {
        return {};
    }

    // Partial overlap: Up to 4 rectangles may result

    // Left part
    if (r.left() > this->left()) {
        result.push_back({this->left(), max2(this->top(), r.top()), r.left() - this->left(), min2(this->bottom(), r.bottom()) - max2(this->top(), r.top())});
    }

    // Right part
    if (r.right() < this->right()) {
        result.push_back({r.right(), max2(this->top(), r.top()), this->right() - r.right(), min2(this->bottom(), r.bottom()) - max2(this->top(), r.top())});
    }

    // Top part
    if (r.top() > this->top()) {
        result.push_back({this->left(), this->top(), this->width(), r.top() - this->top()});
    }

    // Bottom part
    if (r.bottom() < this->bottom()) {
        result.push_back({this->left(), r.bottom(), this->width(), this->bottom() - r.bottom()});
    }

    return result;
}

// Merge adjacent or overlapping rectangles
std::vector<Rect> merge_rectangles(std::vector<Rect> &parts) {

    std::sort(parts.begin(), parts.end(), [](const Rect &a, const Rect &b) { return (a.top() == b.top()) ? a.left() < b.left() : a.top() < b.top(); });

    std::vector<Rect> merged;
    for (auto &part : parts) {
        if (merged.empty()) {
            merged.push_back(part);
        } else {
            Rect &last = merged.back();

            if (last.left() == part.left() && last.width() == part.width() && (last.bottom() == part.top() || last.top() == part.bottom())) {
                last.set_height(last.height() + part.height());
            } else if (last.top() == part.top() && last.height() == part.height() && (last.right() == part.left() || last.left() == part.right())) {
                last.set_width(last.width() + part.width());
            } else {
                merged.push_back(part);
            }
        }
    }

    return merged;
}

Area to_area(Rect &r) { return {{(int16_t)r.left(), (int16_t)r.top(), (uint16_t)r.width(), (uint16_t)r.height()}, (uint16_t)(r.width() * r.height()), 0, 0}; }
