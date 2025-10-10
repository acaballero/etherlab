
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
#include "stm32f4xx_hal_wwdg.h"
#include "view.h"
#include "setup.h"
#include "config.h"
#include "dsp/dsp_tasks.h"
#include <algorithm>
#include <stdint.h>
#include <vector>

bool View::paint_callback() {

    Rect r = parent_rect();
    bool apply_pad = r.width() <= DISPLAY_X_PIXELS;
    Box current_offset = display->getOffset();

    display->fillBuffer(bg_color);

    if (shadow_width) {
        for (int i = 0; i < shadow_width; i++) {
            display->writeRect(i, i, area.box.width - (i + 1), area.box.height - (i + 1), C565_BLACK);
        }
    }

    if (border_width) {
        for (int i = shadow_width; i < border_width + shadow_width; i++) {
            display->writeRect(i, i, area.box.width - (i + 1), area.box.height - (i + 1), border_color);
        }
    }

    // To prevent flickr we have to paint all the children in the callback loop.
    // Otherwise the area will be drawn black then the widgets will be drawn

    // However, if the View does not draw anything by itself (the widgets inside it do)
    // and just clears the background (like this base class does),
    // we can spare from calling children's callback if we draw the background just once (don't set dirty every before_paint)
    // There will be one flickr, but that's all

    // FIXME: This omits calling chidren's before_paint right before rendering, and can lead to them using display settings from the parent's
    // paint_callback execution.

    for (const auto child : this->children()) {
        if (child->can_be_seen()) {

            Rect rect = child->parent_rect();
            Box offset = getOffset(rect, current_offset, !apply_pad);

            if (display->current_line <= offset.y + offset.height - 1 && display->current_last_line >= offset.y) {
                display->setOffset(offset);
                child->paint_callback();
            }
        }
    }

    display->setOffset(current_offset);

    return true;
}

void View::set_parent_rect(Rect r) {
    Widget::set_parent_rect(r);
    // Update children areas
    for (const auto child : this->children()) {
        child->set_parent_rect(child->parent_rect());
    }
}

void View::paint(Area *area) {

    if (this->can_be_seen()) {
        before_paint();

        if (this->dirty()) {

            for (const auto child : this->children()) {
                if (child->can_be_seen()) {
                    child->set_dirty();
                    child->before_paint();
                }
            }

            if (!area && !this->visible_rects.empty()) {
                this->paint_overlapped();
                return;
            }

            if (!area) {
                area = &this->area;
            }

            bool apply_pad = this->parent_rect().width() <= DISPLAY_X_PIXELS;

            // TODO: Take into account if we've received another 'Area' as parameter, other than the full view area
            // LOG("Drawing view %s\n", get_name());
            display->drawArea(area, this, apply_pad);

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
                    child->paint();
                    child->set_clean();
                }
            }
        }
    }
}

void View::add_child(Widget *const widget) {
    if (widget) {
        if (widget->parent() == this) {
            return;
        }

        if (widget->parent() != nullptr) {
            ((View *)widget->parent())->remove_child(widget);
        }

        //   printf_("Adding child %s to %s\n", widget->get_name(), name);
        children_.push_back(widget);

        widget->set_parent(this);
    }
}

void View::on_child_update(Widget *w) {
    // Sort by z-index (ascending order)
    std::sort(children_.begin(), children_.end(), [](const Widget *a, const Widget *b) {
        return a->get_z_index() < b->get_z_index(); // Ascending order
    });

    LOG("on_child_update(%s)\n", w->get_name());
    for (uint16_t i = 0; i < children_.size(); i++) {
        Widget *widget = children_[i];

        widget->visible_rects = {widget->screen_rect()};

        if (STR_IN(widget->get_name(), "snr")) {
            LOG("Cleared %s visible parts\n", widget->get_name());
        }
        // To improve performance, a "sweeping algorightm" can be used (see commented method at the end of the file)

        // std::vector<Rect> visible_parts = {widget->screen_rect()};
        bool overlapped = false;

        for (uint16_t j = i + 1; j < children_.size(); j++) {
            Widget *sibling = children_[j];
            if (sibling->visible() && sibling->get_z_index() >= get_z_index()) {

                Rect r = widget->clip(sibling->screen_rect());
                if (!r.is_empty()) {

                    overlapped = true;

                    //  if (STR_IN(widget->get_name(), "snr")) {
                    if (r.contains(widget->screen_rect())) {
                        LOG("Widget %s hidden by %s\n", widget->get_name(), sibling->get_name());
                    } else {
                        LOG("Widget %s (%d) overlapped by %s (%d)\n", widget->get_name(), widget->get_z_index(), sibling->get_name(), sibling->get_z_index());
                    }
                    //  }
                }
            }
        }

        if (widget->visible_rects.size() == 0) {
            // Note a widget may be partially hidden by several widgets, but completelly by all of them
            if (!widget->hidden()) {
                widget->hidden(true);
            }
        } else {
            if (widget->hidden()) {
                widget->hidden(false);
            }
        }

        if (!overlapped) {
            widget->visible_rects.clear();
        }
    }
}

void View::add_children(const std::initializer_list<Widget *> children) {
    for (auto child : children) {
        add_child(child);
    }
}

bool View::remove_child(Widget *const widget) {
    if (widget) {
        auto it = std::remove(children_.begin(), children_.end(), widget);
        if (it != children_.end()) {
            children_.erase(it, children_.end());
            widget->set_parent(nullptr);

            return true;
        }
    }
    return false;
}

void View::set_area() {

    Widget::set_area();

    // Recalculate children display area
    for (const auto child : this->children()) {
        child->set_parent_rect(child->parent_rect());
    }
}

void View::to_top(Widget *widget) {
    int max_z_index = 0;
    for (auto w : children()) {
        if (w != widget && w->get_z_index() > max_z_index) {
            max_z_index = w->get_z_index();
        }
    }

    widget->set_visible(true);
    widget->set_z_index(max_z_index + 1);
    widget->set_focus(true);
}

const std::vector<Widget *> &View::children() const {
    return children_;
}

bool View::on_input(const st_inputEvent event) {
    return Widget::on_input(event);
}

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
