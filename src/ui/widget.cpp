

#include <algorithm>
#include "Display_afb.h"
#include "widget.h"
#include "ui_types.h"
#include "status.h"

const std::vector<Widget *> Widget::no_children{};

Point Widget::screen_pos() { return screen_rect().location(); }

Size Widget::size() const { return _parent_rect.size(); }

Rect Widget::screen_rect() const { return parent() ? (parent_rect() + parent()->screen_pos()) : parent_rect(); }

Rect Widget::parent_rect() const { return _parent_rect; }

void Widget::set_parent_rect(const Rect new_parent_rect) {

    if (_parent_rect != new_parent_rect) {
        _parent_rect = new_parent_rect;
        // printf_("Setting parent rect of %s\n", name);
        if (parent_) {
            parent_->on_child_update(this);
        }
    }

    this->set_area();
    set_dirty();
}

Widget *Widget::parent() const { return parent_; }

void Widget::set_parent(Widget *const new_parent) {
    if (new_parent == parent_) {
        return;
    }

    if (parent_ && !new_parent) {
        // We have a parent, but are losing it. Update visible status.
        //  dirty_overlapping_children_in_rect(screen_rect());
        set_visible(false);

        parent_->on_child_update(this);
    }

    parent_ = new_parent;

    if (parent_) {
        parent_->on_child_update(this);
    }

    // printf_("Setting parent of %s = %s\n", name, new_parent->name);

    // Adjust drawing area coordinates relative to the parent
    set_area();

    set_dirty();
}

char *Widget::get_name() { return name; }

void Widget::set_dirty() {
    if (!flags.dirty) {
        flags.dirty = true;
        //  printf_("widget %s is dirty\n", name);
    }
}

bool Widget::dirty() const { return flags.dirty; }

void Widget::set_clean() {

    if (flags.dirty) {
        flags.dirty = false;
        //  printf_("widget %s is NOT dirty\n", name);
    }
}

void Widget::hidden(bool hide) {
    if (hide != flags.hidden) {

        // printf_("widget %s hidden: %b\n", name, hide);

        flags.hidden = hide;

        // If parent is hidden, either of these is a no-op.
        if (hide) {
            //  parent()->dirty_overlapping_children_in_rect(parent_rect());
            /* TODO: Notify self and all non-hidden children that they're
             * now effectively hidden?
             */
        } else {
            set_dirty();
            /* TODO: Notify self and all non-hidden children that they're
             * now effectively shown?
             */
        }
    }
}

bool Widget::on_input(const st_inputEvent event) {

    if (!visible() || !enabled()) {
        return false;
    }
    bool consumed = false;

    for (const auto child : children()) {
        if (child->is_focused()) {
            consumed = child->on_input(event);
        }
    }
    return consumed;
}

const std::vector<Widget *> &Widget::children() const { return no_children; }

bool Widget::is_focused() const { return this->flags.focus; }

void Widget::focus(Widget *widget) {

    if (widget) {
        bool is_child = std::find(children().begin(), children().end(), widget) != children().end();

        if (is_child) {
            // Remove focus from other children
            for (const auto child : children()) {
                if (child != widget) {
                    child->set_focus(false);
                }
            }

            // Sets self focus
            set_focus(true);
        }
    }
}

bool Widget::set_focus(bool v) {

    if (v && !visible()) {
        return false;
    }

    if (v != this->flags.focus && this->flags.enabled) {

        // printf_("%s focus = %b\n", name, v);

        this->flags.focus = v;
        if (parent_) {
            if (v) {
                parent_->focus(this);
                this->set_dirty();
                this->on_focus();
            } else {
                // Remove focus from other children
                for (const auto child : children()) {
                    child->set_focus(false);
                }
            }
        }

        if (!v) {
            this->set_dirty();
            this->on_blur();
        }
    }

    return true;
}

Widget *Widget::focused_widget() const {
    for (const auto child : children()) {
        if (child->is_focused()) {
            return child;
        }
    }
    return nullptr;
}

bool Widget::visible() { return this->flags.visible; }

bool Widget::can_be_seen() { return this->flags.visible && !this->flags.hidden; }

void Widget::set_visible(bool v) {

    if (v != flags.visible) {

        // printf_("%s visible = %b\n", name, v);

        flags.visible = v;
        flags.dirty = v;

        /* TODO: This on_show/on_hide implementation seems inelegant.
         * But I need *some* way to take/configure resources when
         * a widget becomes visible, and reverse the process when the
         * widget becomes invisible, whether the widget (or parent) is
         * hidden, or the widget (or parent) is removed from the tree.
         */

        if (v) {
            on_show();
        } else {
            set_focus(false);
            on_hide();
        }

        if (parent_) {
            parent_->on_child_update(this);
        }
    }
}

Display *Widget::get_display() const { return display; }

void Widget::set_display(Display *display) { Widget::display = display; }

void Widget::paint(Area *area) {

    // update_overlaps();

    before_paint(); // pure virtual

    if (this->dirty()) {

        bool apply_pad = this->parent_rect().width() <= DISPLAY_X_PIXELS;

        if (!area) {
            area = &this->area;
        }

        display->drawArea(area, this, apply_pad);

#if DEBUG_LCD
        uint64_t t = HAL_GetTick();
        // TODO: This whole "area" thing (needed to adapt the display driver double buffering interface) is redundant (we already have the parent rect) and
        // unelegant
        this->area.fps = this->fps;
        this->area.show_fps = this->show_fps;

        if (this->show_fps) {
            if (this->last_refresh_ms) {
                float fps = 1000.0 / (float)(t - this->last_refresh_ms);
                this->fps = this->fps - (0.3 * (this->fps - fps));
            }
        }
        this->last_refresh_ms = t;
#endif
    }
}

void Widget::set_font(FontDef *font) { Widget::font = font; }

void Widget::set_aling(Align a) { align = a; }

void Widget::set_show_fps(bool b) {
    this->show_fps = b;
    this->area.show_fps = b;
}

void Widget::set_area() {

    Rect r = screen_rect();

    area = {{(int16_t)r.left(), (int16_t)r.top(), (uint16_t)r.width(), (uint16_t)r.height()}, (uint16_t)(r.width() * r.height()), this->show_fps, this->fps};
}

uint8_t Widget::get_z_index() const { return z_index + (parent() ? parent()->get_z_index() : 0); }

void Widget::set_z_index(uint8_t index) {
    Widget::z_index = index;
    if (parent()) {
        parent()->on_child_update(this);
    }
}

void Widget::set_name(const char *str) { snprintf(name, sizeof(name), str); }

bool Widget::active() { return flags.active; }

void Widget::set_active(bool v) { flags.active = v; }

bool Widget::enabled() { return flags.enabled; }

void Widget::set_enabled(bool v) { flags.enabled = v; }
