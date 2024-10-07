

#include <algorithm>
#include "widget.h"
#include "ui_types.h"

const std::vector<Widget *> Widget::no_children{};

Point Widget::screen_pos() {
    return screen_rect().location();
}

Size Widget::size() const {
    return _parent_rect.size();
}

Rect Widget::screen_rect() const {
    return parent() ? (parent_rect() + parent()->screen_pos()) : parent_rect();
}

Rect Widget::parent_rect() const {
    return _parent_rect;
}

void Widget::set_parent_rect(const Rect new_parent_rect) {
    _parent_rect = new_parent_rect;
    this->set_area();
    set_dirty();
}

Widget *Widget::parent() const {
    return parent_;
}

void Widget::set_parent(Widget *const widget) {
    if (widget == parent_) {
        return;
    }

    if (parent_ && !widget) {
        // We have a parent, but are losing it. Update visible status.
        dirty_overlapping_children_in_rect(screen_rect());
        set_visible(false);
    }

    parent_ = widget;

    // Adjust drawing area coordinates relative to the parent
    set_area();

    set_dirty();
}

void Widget::set_dirty() {
    flags.dirty = true;
}

bool Widget::dirty() const {
    return flags.dirty;
}

void Widget::set_clean() {
    flags.dirty = false;
}

void Widget::hidden(bool hide) {
    if (hide != flags.hidden) {
        flags.hidden = hide;

        // If parent is hidden, either of these is a no-op.
        if (hide) {
            parent()->dirty_overlapping_children_in_rect(parent_rect());
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

    if (!visible() || !enabled()) return false;

    bool consumed = false;

    for (const auto child: children()) {
        if (child->is_focused()) {
            consumed = child->on_input(event);
        }
    }
    return consumed;
}

const std::vector<Widget *> &Widget::children() const {
    return no_children;
}

bool Widget::is_focused() const {
    return this->flags.focus;
}

void Widget::focus(Widget *widget) {

    if (widget) {
        bool is_child = std::find(children().begin(), children().end(), widget) != children().end();

        if (is_child) {
            // Remove focus from other children
            for (const auto child: children()) {
                if (child != widget) {
                    child->set_focus(false);
                }
            }

            // Sets self focus
            set_focus(true);
        }
    }
}

void Widget::set_focus(bool v) {

    if (v != this->flags.focus && this->flags.enabled) {
        this->flags.focus = v;
        if (parent_) {
            if (v) {
                parent_->focus(this);
            } else {
                // Remove focus from other children
                for (const auto child: children()) {
                    child->set_focus(false);
                }
            }
        }
        this->set_dirty();
    }
}

Widget *Widget::focused_widget() const {
    for (const auto child: children()) {
        if (child->is_focused()) {
            return child;
        }
    }
    return nullptr;
}

bool Widget::visible() {
    return this->flags.visible;
}

void Widget::set_visible(bool v) {
    if (v != flags.visible) {

        flags.visible = v;
        flags.dirty = v;

        set_focus(false);

        /* TODO: This on_show/on_hide implementation seems inelegant.
         * But I need *some* way to take/configure resources when
         * a widget becomes visible, and reverse the process when the
         * widget becomes invisible, whether the widget (or parent) is
         * hidden, or the widget (or parent) is removed from the tree.
         */
        if (v) {
            on_show();
        } else {
            on_hide();

            // Set all children invisible too.
            //for (const auto child: children()) {
            //    child->set_visible(false);
            //}
        }
    }
}

void Widget::dirty_overlapping_children_in_rect(const Rect &child_rect) {
    for (auto child: children()) {
        if (!child_rect.intersect(child->parent_rect()).is_empty()) {
            child->set_dirty();
        }
    }
}

Display *Widget::get_display() const {
    return display;
}

void Widget::set_display(Display *display) {
    Widget::display = display;
}

void Widget::paint() {

    // update_overlaps();

    do_paint(); // pure virtual

#if DEBUG_LCD
    if (this->dirty()) {
        uint64_t t = HAL_GetTick();
        // TODO: This whole "area" thing (needed to adapt the display driver double buffering interface) is redundant and unelegant
        this->area.fps = this->fps;
        this->area.show_fps = this->show_fps;

        if (this->show_fps) {
            if (this->last_refresh_ms) {
                float fps = 1000.0 / (float) (t - this->last_refresh_ms);
                this->fps = this->fps - (0.3 * (this->fps - fps));
            }
        }
        this->last_refresh_ms = t;
    }
#endif
}

void Widget::set_font(FontDef *font) {
    Widget::font = font;
}

void Widget::set_aling(Align a) {
    align = a;
}

void Widget::set_show_fps(bool b) {
    this->show_fps = b;
    this->area.show_fps = b;
}

void Widget::set_area() {

    Rect r = screen_rect();

    area = {
            (uint16_t) r.left(),
            (uint16_t) r.top(),
            (uint16_t) r.width(),
            (uint16_t) r.height(),
            (uint16_t) (r.width() * r.height()),
            this->show_fps,
            this->fps
    };
}

uint8_t Widget::get_z_index() const {
    return z_index;
}

void Widget::set_z_index(uint8_t index) {
    Widget::z_index = index;
}

void Widget::update_overlaps() {

    if (!this->parent()) {
        return;
    }

    //std::vector<Rect &> overlaps;
    std::vector<Widget *> children = this->parent()->children();
    for (uint16_t i = 0; i < children.size(); i++) {
        Widget *child = children[i];
        if (child->get_z_index() > z_index && child->visible()) {
            const Rect r = this->screen_rect().intersect(child->screen_rect());
            if (!r.is_empty()) {
                //overlaps.push_back(move(r));
                child->set_dirty();
            }
        }
    }
}

bool Widget::active() {
    return flags.active;
}

void Widget::set_active(bool v) {
    flags.active = v;
}

bool Widget::enabled() {
    return flags.enabled;
}

void Widget::set_enabled(bool v) {
    flags.enabled = v;
}


