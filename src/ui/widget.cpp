

#include <algorithm>
#include <cstddef>
#include <cstring>

#include "Display_afb.h"
#include "input/inputEvent.h"
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

void Widget::set_width(int w) {
    auto pr = parent_rect();
    pr.set_width(w);
    set_parent_rect(pr);
}

void Widget::set_height(int h) {

    auto pr = parent_rect();
    pr.set_height(h);
    set_parent_rect(pr);
}

void Widget::set_top(int y) {
    auto pr = parent_rect();
    pr.set_top(y);
    set_parent_rect(pr);
}

void Widget::set_left(int x) {
    auto pr = parent_rect();
    pr.set_left(x);
    set_parent_rect(pr);
}

void Widget::set_bg(Color c) {
    bg_color = c;
}

Color Widget::get_bg() {
    return bg_color;
}

uint16_t Widget::get_fg() const {
    return fg_color;
}

void Widget::set_fg(uint16_t fg) {
    fg_color = fg;
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
        parent_->on_child_update(this); // Make the parent react
    }

    parent_ = new_parent;

    if (parent_ && can_be_seen()) {
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

        // #if DEBUG_MSGS
        //         if (STR_IN(get_name(), "info", "smet", "radi", "snr")) {
        //             LOG("Widget %s hidden: %d\n", get_name(), hide);
        //         }
        // #endif

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

    // LOG("Checking %s %d visible rects\n", get_name(), visible_rects.size());
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

    if (!event.is_touch()) {

        auto w = focused_widget();

        // Bubble from the focused descendant until it is consumed
        while (w && !consumed && w != this) {
            // LOG("Child %s focused\n", w->get_name());
            consumed = w->on_input(event);
            w = w->parent();
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

Widget *Widget::get_focusable_widget(Widget *root) {
    if (root == nullptr) {
        return nullptr;
    }

    if (!root->can_be_seen()) {
        return nullptr;
    }

    if (root->focusable()) {
        return root;
    }

    for (auto it = root->children().rbegin(); it != root->children().rend(); ++it) { // traverse in reverse z-index order (decreasing)
        Widget *result = get_focusable_widget(*it);

        if (result != nullptr) {
            return result;
        }
    }

    return nullptr;
}

void Widget::refocus() {
    // for (auto *w : children()) {
    //   LOG("%s,%s ID:%d\n", get_name(),w->get_name(), w->get_z_index());
    // }

    if (!focused_widget()) {
        // Lost focused widget, focus the first focusable widget

        //  LOG("refocus: %s lost focused widget\n", get_name());

        Widget *focusable_w = get_focusable_widget(this);

        if (focusable_w) {
            //  LOG("Found focusable widget %s\n", focusable_w->get_name());
            focusable_w->set_focus(true);
        }
    }
}

void Widget::on_child_focus_changed(Widget *widget, bool was_focused) {

    if (widget) {
        bool is_child = std::find(children().begin(), children().end(), widget) != children().end();

        if (is_child) {
            // Remove focus from other children
            if (widget->is_focused() || widget->focused_widget()) {
                // if (widget->is_focused()) {
                //     LOG("child %s of %s has focused\n", widget->get_name(), get_name());
                // }
                // if (widget->focused_widget()) {
                //     LOG("child %s of %s contains focused widget %s\n", widget->get_name(), get_name(), widget->focused_widget()->get_name());
                // }
                for (const auto child : children()) {
                    if (child != widget) {
                        child->set_focus(false);
                    }
                }
            }

            // Tells parent
            if (parent_) {
                parent_->on_child_focus_changed(widget, was_focused);
            }
        }
    }
}

bool Widget::set_focus(bool v) {

    // LOG("'%s' focus: %d => %d\n", get_name(), flags.focus, v);

    if (!v) {
        // int i = 0;
        //  Remove focus from its children
        for (const auto child : children()) {
            // LOG_RAW("[%d]", i++);
            child->set_focus(false);
        }
    }

    if (!flags.focusable) {

        if (v && !focused_widget()) {
            // The widget is not itself focusable but if it has no focused widget, will try to focus on the default
            Widget *focusable_child = get_focusable_widget(this);
            if (focusable_child) {
                return focusable_child->set_focus(v);
            }
        }

        return false;
    }

    if (v && !can_be_seen()) {

        return false;
    }

    if (v != this->flags.focus && this->flags.enabled) {

        //  LOG("%s focus = %b\n", name, v);

        bool was_focused = flags.focus;

        this->flags.focus = v;

        this->set_dirty();

        if (parent_) {
            // if (v) {
            parent_->on_child_focus_changed(this, was_focused);
            // }
        }

        if (!v) {
            this->on_blur();
        } else {
            on_focus();
        }

        if (on_focus_fn) {
            on_focus_fn();
        }
    }

    return true;
}

Widget *Widget::focused_widget(bool recursive) const {
    for (const auto child : children()) {

        if (child->is_focused() || (!recursive && child->focused_widget())) {
            return child;
        } else if (recursive) {
            Widget *w = child->focused_widget();
            if (w) {
                return w;
            }
        }

        // Deepest first
        // Widget *w = child->focused_widget();
        // if (w) {
        //     return w;
        // } else if (child->is_focused()) {
        //     return child;
        // }
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
        if (parent_) {
            parent_->on_child_update(this);
        }

        if (v) {
            on_show();
        } else {

            bool was_focused = is_focused() || focused_widget();
            set_focus(false);

            if (parent_ && was_focused) {
                //  LOG("visibility lost\n");
                parent_->refocus();
            }

            on_hide();
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

        // LOG("Drawing widget %s\n", get_name());

        display->draw_area(area, this, apply_pad);

        refresh_fps();
    }
}

void Widget::paint_overlapped() {

    Box current_offset = display->get_offset();

    // if (strcmp("msg", get_name()) == 0 || strcmp("waterfall", get_name()) == 0 || strcmp("radio", get_name()) == 0) {
    //     LOG("[paint_overlapped] Child %s has %d visible rect/s\n", get_name(), visible_rects.size());
    // }
    for (auto &rect : visible_rects) {

        Rect pr = parent_rect();
        Rect sr = screen_rect();

        // Currenty only full width rects are considered
        if (rect.width() > pr.width() / 3) {

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

            // if (strcmp("msg", get_name()) == 0 || strcmp("waterfall", get_name()) == 0 || strcmp("radio", get_name()) == 0) {
            //     printf_("Painting area (%d,%d,%d,%d), offset (%d,%d,%d,%d) of widget %s\n", a.box.x, a.box.y, a.box.width, a.box.height, offset.x,
            //     offset.y,
            //             offset.width, offset.height, get_name());
            // }
            paint(&a);

        } else {
            // if (strcmp("msg", get_name()) == 0 || strcmp("waterfall", get_name()) == 0 || strcmp("radio", get_name()) == 0) {
            //     LOG("Child %s has a 'narrow' visible part\n", get_name());
            // }
            //   printf_("Rect: (%d,%d,%d,%d) of widget %s\n", rect.left(), rect.top(), rect.width(), rect.height(), child->get_name());
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

void Widget::set_font(const FontDef *font) {
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
    if (z_index != index) {
        Widget::z_index = index;
        if (parent()) {
            parent()->on_child_update(this);
        }
    }
}

Rect Widget::clip(const Rect &rect) {

    std::vector<Rect> parts = visible_rects;
    bool overlapped = false;

    if (parts.size() == 0) {
        parts = {screen_rect()};
        // #if DEBUG_MSGS
        //         if (STR_IN(get_name(), "aprs", "msg")) {
        //             LOG("Using screen rect as parts %d,%d %d x %d\n", parts[0].left(), parts[0].top(), parts[0].width(), parts[0].height());
        //         }
        // #endif
    }

    const Rect r = screen_rect().intersect(rect);
    if (!r.is_empty()) {

        overlapped = true;
        std::vector<Rect> new_visible_parts;
        for (auto &part : parts) {
            std::vector<Rect> subtracted = (part - rect);
            new_visible_parts.insert(new_visible_parts.end(), subtracted.begin(), subtracted.end());
        }
        parts = new_visible_parts;

#if DEBUG_MSGS
        // if (STR_IN(get_name(), "powm", "brpt", "bscn", "fbut")) {
        //     LOG("Clipping widget %s with rect %d,%d", get_name(), rect.left(), rect.top());
        //     LOG_RAW(" %d x %d\n", rect.width(), rect.height());
        //     LOG("Current parts (%d)\n", visible_rects.size());
        //     if (visible_rects.size()) {
        //         for (auto p : visible_rects) {
        //             LOG("%d,%d %d x %d\n", p.left(), p.top(), p.width(), p.height());
        //         }
        //     } else {
        //     }

        //     LOG("New parts (%d)\n", parts.size());
        //     if (parts.size()) {
        //         for (auto p : parts) {
        //             LOG("%d,%d %d x %d\n", p.left(), p.top(), p.width(), p.height());
        //         }
        //     }
        // }
#endif
    }

    if (parts.size() == 0) { // Widget is now hidden
        hidden(true);
    }

    if (overlapped) {
        visible_rects = parts;
    }

    // Recursively clip children
    for (const auto child : this->children()) {
        child->clip(rect);
    }

    return r;
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
