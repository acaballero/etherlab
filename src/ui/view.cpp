
#include <dsp/dsp.h>
#include "view.h"
#include "../../lib/utils/utils.hpp"
#include "Display_afb.h"
#include "dsp/fft/fft.h"
#include "dsp/dsp.h"
#include "status.h"
#include "../../lib/printf/printf.h"
#include "../../lib/ST77XX-STM32/ILI9341_fb.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_def.h"
#include "view.h"
#include "setup.h"
#include "config.h"
#include "dsp/dsp_tasks.h"
#include <algorithm>
#include <stdint.h>
#include <vector>

void View::paint_callback() {

    bool apply_pad = this->parent_rect().width() <= DISPLAY_X_PIXELS;
    Box offset = display->getOffset();

    display->clear();

    // To prevent flickr we have to paint all the children in the callback loop.
    // Otherwise the area will be drawn black then the widgets will be drawn

    // However, if the View does not draw anything by itself (the widgets inside it do)
    // and just clears the background (like this base class does),
    // we can spare from calling children's callback if we draw the background just once (don't set dirty every before_paint)
    // There will be one flickr, but that's all

    for (const auto child : this->children()) {
        if (child->can_be_seen()) {
            uint16_t top = child->parent_rect().top();
            uint16_t left = child->parent_rect().left();
            uint16_t height = child->parent_rect().height();
            uint16_t width = child->parent_rect().width();

            // top = top ? top - 1 : 0;

            if (!apply_pad) {
                top += DISPLAY_PADDING;
                left += DISPLAY_PADDING;
            }

            // Add current offset
            left += offset.x;
            top += offset.y;

            if (display->current_line <= top + height - 1 && display->current_last_line >= top) {
                display->setOffset({left, top, width, height});
                child->paint_callback();
            }
        }
    }

    display->setOffset(offset);
}

void View::set_parent_rect(Rect r) {
    Widget::set_parent_rect(r);
    // Update children areas
    for (const auto child : this->children()) {
        child->set_parent_rect(child->parent_rect());
    }
}

void View::paint(Area *) {

    if (this->can_be_seen()) {
        before_paint();

        if (this->dirty()) {

            for (const auto child : this->children()) {
                if (child->can_be_seen()) {
                    child->set_dirty();
                    child->before_paint();
                }
            }

            bool apply_pad = this->parent_rect().width() <= DISPLAY_X_PIXELS;
            display->drawArea(&this->area, this, apply_pad);

            for (const auto child : this->children()) {
                if (child->can_be_seen()) {
                    child->set_clean();
                }
            }

            this->set_clean();

        } else {

            // Selectively paint all children.
            for (const auto child : this->children()) {
                if (child->can_be_seen()) {

                    std::vector<Widget *> overlaps = this->overlap_map[child];

                    if (overlaps.empty()) {
                        // if (strcicmp(child->get_name(), "fft") == 0) {
                        //     printf_("%s has no overlaps\n", child->get_name());
                        // }
                        child->paint();
                        child->set_clean();
                    } else {
                        // if (strcicmp(child->get_name(), "fft") == 0) {
                        //     printf_("%s has %d overlaps\n", child->get_name(), overlaps.size());
                        //}
                        // If there's a partially covered (and dirty) widget, we need (if we would paint it entirelly) to also paint the overlapping area of the
                        // widgets on top of it.

                        // Unfortunatelly, we can't just paint the affected widgets, or the background in-between wouldn't be drawn, so we need to repaint the
                        // hole view, only that we will paint just the minimum required vertical area.

                        // There are some easy optimizations we can do:
                        // If the seen portion of the widget is a rectangle, we could just paint that box. Knowing if that's the case would require using
                        // "sweeping" methods to account for all cases, but one frequent case is when a widget is partially covered by just another one,
                        // horizontally (or vertically <-- Well, not really. The way the "shared-buffered-multiple-pass" paint method currently works is not
                        // possible
                        // to paint buffers with x dimension smaller that its original width).

                        if (overlaps.size() == 1) {
                            Widget *w = overlaps.at(0);
                            Rect pr = child->parent_rect();
                            Rect sr = child->screen_rect();
                            if (w->parent_rect().left() <= pr.left() && w->parent_rect().right() >= pr.right()) {

                                // Partially covered horizontally by only one widget
                                std::vector<Rect> rects = sr - w->screen_rect();
                                for (auto rect : rects) {
                                    Area a = to_area(rect);

                                    if (sr.top() < rect.top()) {
                                        // TODO:: The rectangle to paint has an offset
                                        status::handleError(status::ST_ERROR, "Negative offset not implemented");
                                    }

                                    printf_("Painting area (%d,%d,%d,%d) of widget %s\n", a.box.x, a.box.y, a.box.width, a.box.height, child->get_name());
                                    child->paint(&a);
                                }
                            }
                        }

                        child->set_clean();
                    }
                }
            }
        }
    }
}

void View::add_child(Widget *const widget) {
    if (widget) {
        if (widget->parent() == nullptr) {
            printf_("Adding child %s to %s\n", widget->get_name(), name);
            children_.push_back(widget);
            widget->set_parent(this);
        }
    }
}

void View::on_child_update(Widget *w) {
    // Sort by z-index (ascending order)
    std::sort(children_.begin(), children_.end(), [](const Widget *a, const Widget *b) {
        return a->get_z_index() < b->get_z_index(); // Ascending order
    });

    for (uint16_t i = 0; i < children_.size(); i++) {
        Widget *widget = children_[i];

        std::vector<Widget *> &overlaps = overlap_map[widget];
        overlaps.clear();

        // TODO: This maximum overlapping rectangle does not work as soon as the partial overlaps are not contiguous or leave gaps
        // For example:
        //
        // ---------
        // | 1 | 2 |
        // ---------
        // | 3 |   .
        // -----....
        //
        // A rectangle under those three (dots) will be detected as covered
        // To overcome this, a "sweeping algorightm" can be used (see commented method at the end of the file)
        Rect max_overlapping_rect{};

        for (uint16_t j = i + 1; j < children_.size(); j++) {
            Widget *sibling = children_[j];
            if (sibling->visible() && sibling->get_z_index() >= get_z_index()) {
                const Rect r = widget->screen_rect().intersect(sibling->screen_rect());
                if (!r.is_empty()) {
                    max_overlapping_rect += r;
                    if (r.contains(widget->screen_rect())) {
                        //        printf_("Widget %s hidden by %s\n", widget->get_name(), sibling->get_name());
                    } else {
                        printf_("Widget %s overlapped by %s\n", widget->get_name(), sibling->get_name());
                        // Process the overlap in the widget's childs to see if some can be hidden
                    }
                    overlaps.push_back(sibling);
                }
            }
        }

        if (max_overlapping_rect.contains(widget->screen_rect())) {
            // Note a widget may be partially hidden by several widgets, but completelly by all of them
            if (!widget->hidden()) {
                widget->hidden(true);
            }
        } else {
            if (widget->hidden()) {
                widget->hidden(false);
            }
        }
    }
}

void View::add_children(const std::initializer_list<Widget *> children) {
    for (auto child : children) {
        add_child(child);
    }
}

void View::remove_child(Widget *const widget) {
    if (widget) {
        children_.erase(std::remove(children_.begin(), children_.end(), widget), children_.end());
        widget->set_parent(nullptr);
    }
}

void View::set_area() {

    Widget::set_area();

    // Recalculate children display area
    for (const auto child : this->children()) {
        child->set_parent_rect(child->parent_rect());
    }
}

const std::vector<Widget *> &View::children() const { return children_; }

bool View::on_input(const st_inputEvent event) { return Widget::on_input(event); }

void View::on_hide() {
    if (on_hide_fn) {
        on_hide_fn();
    }
}

// // Structure to store vertical events for plane sweep
// struct Event {
//     int x, y_start, y_end, type; // type: 1 for start, -1 for end
//     bool operator<(const Event& e) const {
//         if (x == e.x) return type > e.type; // Start events before end events at the same x
//         return x < e.x;
//     }
// };

// // Plane sweep algorithm to check full coverage
// bool is_fully_covered(const Rect& target, const std::vector<Rect>& rects) {
//     std::vector<Event> events;

//     // Collect all intersections
//     for (const Rect& r : rects) {
//         Rect inter = target.intersect(r);
//         if (inter.area() > 0) {
//             events.push_back({inter.x, inter.y, inter.y + inter.height, 1});
//             events.push_back({inter.x + inter.width, inter.y, inter.y + inter.height, -1});
//         }
//     }

//     // Sort events by x coordinate
//     std::sort(events.begin(), events.end());

//     // Sweep line algorithm
//     std::multiset<std::pair<int, int>> active_intervals;
//     int prev_x = target.x;
//     int covered_y_length = 0;

//     for (const auto& e : events) {
//         int dx = e.x - prev_x;

//         // Check if the entire height of the target is covered
//         if (covered_y_length == target.height && dx > 0) {
//             prev_x = e.x;
//         } else if (dx > 0) {
//             return false;  // Found an uncovered vertical strip
//         }

//         if (e.type == 1) {
//             active_intervals.insert({e.y_start, e.y_end});
//         } else {
//             active_intervals.erase(active_intervals.find({e.y_start, e.y_end}));
//         }

//         // Compute total covered height
//         int last_y = -1;
//         covered_y_length = 0;
//         for (const auto& [y_start, y_end] : active_intervals) {
//             if (y_start > last_y) {
//                 covered_y_length += y_end - y_start;
//                 last_y = y_end;
//             } else if (y_end > last_y) {
//                 covered_y_length += y_end - last_y;
//                 last_y = y_end;
//             }
//         }

//         prev_x = e.x;
//     }

//     return covered_y_length == target.height;
// }
