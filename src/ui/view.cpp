
#include <dsp/dsp.h>
#include "view.h"
#include "../../lib/utils/utils.hpp"
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

void View::paint_callback() {
    display->clear();
}

void View::paint() {

    if( this->flags.visible ) {

        if( this->flags.dirty ) {

            // Clear background
            display->drawArea(&this->area,this);

            // Force-paint all children.
            for(const auto child : this->children()) {
                if (child->visible()) {
                    child->set_dirty();
                    child->paint();
                    child->set_clean();
                }
            }

            this->flags.dirty=false;

        } else {
            // Selectively paint all children.
            for(const auto child : this->children()) {
                if (child->visible()) {
                    child->paint();
                    child->set_clean();
                }
            }
        }
    }

    Widget::paint();
}

void View::add_child(Widget* const widget) {
    if( widget ) {
        if( widget->parent() == nullptr ) {
            widget->set_parent(this);
            children_.push_back(widget);
        }
    }
}

void View::add_children(const std::initializer_list<Widget*> children) {
    children_.insert(std::end(children_), children);
    for(auto child : children) {
        child->set_parent(this);
    }
}

void View::remove_child(Widget* const widget) {
    if( widget ) {
        children_.erase(std::remove(children_.begin(), children_.end(), widget), children_.end());
        widget->set_parent(nullptr);
    }
}

const std::vector<Widget*>& View::children() const {
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





