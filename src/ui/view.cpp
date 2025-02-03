
#include <dsp/dsp.h>
#include "view.h"
#include "../../lib/utils/utils.hpp"
#include "Display_afb.h"
#include "dsp/fft/fft.h"
#include "dsp/dsp.h"
#include "status.h"
#include "../../lib/printf/printf.h"
#include "../../lib/ST77XX-STM32/ILI9341_fb.h"
#include "view.h"
#include "setup.h"
#include "config.h"
#include "dsp/dsp_tasks.h"
#include <algorithm>
#include <stdint.h>

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
        if (child->visible()) {
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

void View::paint() {

    if (this->visible()) {
        before_paint();

        if (this->dirty()) {

            for (const auto child : this->children()) {
                if (child->visible()) {
                    child->set_dirty();
                    child->before_paint();
                }
            }

            bool apply_pad = this->parent_rect().width() <= DISPLAY_X_PIXELS;
            display->drawArea(&this->area, this, apply_pad);

            for (const auto child : this->children()) {
                if (child->visible()) {
                    child->set_clean();
                }
            }

            this->set_clean();
        } else {
            // Selectively paint all children.
            for (const auto child : this->children()) {
                if (child->visible()) {
                    child->paint();
                    child->set_clean();
                }
            }
        }
    }
}

void View::add_child(Widget *const widget) {
    if (widget) {
        if (widget->parent() == nullptr) {
            widget->set_parent(this);
            children_.push_back(widget);
        }
    }
}

void View::add_children(const std::initializer_list<Widget *> children) {
    children_.insert(std::end(children_), children);
    for (auto child : children) {
        child->set_parent(this);
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
