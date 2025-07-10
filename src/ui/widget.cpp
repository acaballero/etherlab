

#include <algorithm>
#include "Display_afb.h"
#include "ui/frequency_memory_ui.h"
#include "view.h"
#include "widget.h"
#include "ui_types.h"
#include "status.h"

const std::vector<Widget *> Widget::no_children{};

Point Widget::screen_pos() {
    return screen_rect().location();
}

Size Widget::size() const {
    return _parent_rect.size();
}

Rect Widget::screen_rect() const {
    return parent() ? (parent_rect() + parent()->screen_pos()) : (parent_rect());
}

Rect Widget::parent_rect() const {
    return _parent_rect;
}

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

void Widget::set_bg(Color c) {
    bg_color = c;
}

Widget *Widget::parent() const {
    return parent_;
}

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

char *Widget::get_name() {
    return name;
}

void Widget::set_dirty() {
    if (!flags.dirty) {
        flags.dirty = true;
        //  printf_("widget %s is dirty\n", name);
    }
}

bool Widget::dirty() const {
    return flags.dirty;
}

void Widget::set_clean() {

    if (flags.dirty) {
        flags.dirty = false;
        //  printf_("widget %s is NOT dirty\n", name);
    }
}

void Widget::hidden(bool hide) {
    if (hide != flags.hidden) {

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

bool Widget::is_point_visible(Point &p) {
    if (!can_be_seen()) {
        return false;
    }

    if (!visible_rects.size()) { // not overlapped
        return screen_rect().contains(p);
    }

    for (auto &r : visible_rects) {
        if (r.contains(p) && r.width() > 2 && r.height() > 2) { // we don't consider thin rectangles
            return true;
        }
    }

    return false;
}

bool Widget::on_input(const st_inputEvent event) {

    // printf_("----> Widget %s on_input: %d\n", this->name, event.type);

    if (!visible() || !enabled()) {
        return false;
    }
    bool consumed = false;

    for (const auto child : children()) {
        if (child->is_focused() && !event.is_touch()) {

            // printf_("Child %s focused\n", child->get_name());

            consumed = child->on_input(event);
            if (consumed) { // Only one child should receive the input (break in case another is focused if consumed)
                break;
            }
        }
    }

    if (!consumed) {
        switch (event.type) {

            case INPUT_EVENT_TYPE_TOUCH_END:

                consumed = this->on_touch(event);
                break;

            default:
                break;
        }
    }

    // printf_("<---- Exiting %s on_input: %b\n", this->name, consumed);
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

            if (get_quick_actions()) {
                actions_signal.emit(nullptr);
            }
        } else {
            if (get_quick_actions()) {
                actions_signal.emit(get_quick_actions());
            }
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

bool Widget::visible() {
    return this->flags.visible;
}

bool Widget::can_be_seen() {
    return this->flags.visible && !this->flags.hidden;
}

void Widget::set_visible(bool v) {

    if (v != flags.visible) {

        //   printf_("%s visible = %b\n", name, v);

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

Display *Widget::get_display() const {
    return display;
}

void Widget::set_display(Display *display) {
    Widget::display = display;
}

void Widget::paint(Area *area) {

    before_paint(); // pure virtual

    if (this->dirty()) {

        if (!area && !this->visible_rects.empty()) {
            this->paint_overlapped();
            return;
        }

        if (!area) {
            area = &this->area;
        }

        bool apply_pad = this->parent_rect().width() <= DISPLAY_X_PIXELS;

        display->drawArea(area, this, apply_pad);

        refresh_fps();
    }
}

void Widget::paint_overlapped() {

    Box current_offset = display->getOffset();

    // printf_("Child %s has %d visible rect/s\n", get_name(), visible_rects.size());

    for (auto &rect : visible_rects) {

        Rect pr = parent_rect();
        Rect sr = screen_rect();

        // Currenty only full width rects are considered
        if (rect.width() == pr.width()) {

            Area a = to_area(rect);

            // From screen to relative
            Rect r = rect - sr.location();

            Box offset;

            if (sr.top() < rect.top()) {
                offset = getOffset(r, current_offset, false); // Don't apply pad here, it's taken care off in Widget::paint

                // Negative offset
                offset.x = -offset.x;
                offset.y = -offset.y;
                display->setOffset(offset);
            }

            // printf_("Painting area (%d,%d,%d,%d), offset (%d,%d,%d,%d) of widget %s\n", a.box.x, a.box.y, a.box.width, a.box.height, offset.x, offset.y,
            //         offset.width, offset.height, get_name());

            paint(&a);

        } else {
            // status::handleError(status::ST_ERROR, "A child has a 'small' visible part");
            // printf_("Rect: (%d,%d,%d,%d) of widget %s\n", rect.left(), rect.top(), rect.width(), rect.height(), child->get_name());
        }

        display->setOffset(current_offset);
    }
}

Box Widget::getOffset(Rect &r, Box &offset, bool apply_pad) {

    int16_t top = r.top();
    int16_t left = r.left();
    uint16_t height = r.height();
    uint16_t width = r.width();

    // top = top ? top - 1 : 0;

    if (apply_pad) {
        top += DISPLAY_PADDING;
        left += DISPLAY_PADDING;
    }

    // Add current offset
    left += offset.x;
    top += offset.y;

    return {left, top, width, height};
}

void Widget::refresh_fps() {

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

    area = {{(int16_t)r.left(), (int16_t)r.top(), (uint16_t)r.width(), (uint16_t)r.height()}, (uint16_t)(r.width() * r.height()), this->show_fps, this->fps};
}

uint16_t Widget::get_z_index() const {
    return z_index + (parent() ? parent()->get_z_index() : 0);
}

void Widget::set_z_index(uint16_t index) {
    Widget::z_index = index;
    if (parent()) {
        parent()->on_child_update(this);
    }
}

void Widget::set_name(const char *str) {
    snprintf(name, sizeof(name), str);
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
