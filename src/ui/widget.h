#ifndef __WIDGET_H__
#define __WIDGET_H__

#include "ui_types.h"
#include "../input/inputEvent.h"
#include "../../lib/ST77XX-STM32/Display_afb.h"
#include "../../lib/ST77XX-STM32/Painter.hpp"
#include "../../lib/printf/printf.h"
#include "lcd.h"
#include <vector>
#include <printf.h>
#include "menu_options.h"
#include "status.h"

enum Align { ALIGN_LEFT, ALIGN_RIGHT, ALIGN_CENTER };

class Widget : public Painter {
  public:
    Widget() : _parent_rect{}, display{(Display *)&lcd} {
    }

    Widget(Rect parent_rect, Display *display, const char *name = nullptr) : _parent_rect{parent_rect}, display{display} {
        if (name) {
            set_name(name);
        }
        this->set_area();
    }

    Widget(const Widget &) = delete;

    Widget(Widget &&) = default;

    Widget &operator=(const Widget &) = delete;

    Widget &operator=(Widget &&) = delete;

    virtual ~Widget() = default;

    Point screen_pos();

    Size size() const;

    Rect screen_rect() const;

    Rect parent_rect() const;

    void set_show_fps(bool);

    virtual void set_parent_rect(const Rect new_parent_rect);

    void set_width(int w);

    void set_height(int h);

    void set_top(int y);

    void set_left(int x);

    Widget *parent() const;

    Widget *focused_widget() const;

    virtual void set_parent(Widget *const widget);

    bool hidden() const {
        return flags.hidden;
    }

    void hidden(bool hide);

    // Before paint hook for pre-paint preparation
    virtual void before_paint() = 0;

    virtual void paint(Area *area = nullptr);

    virtual void on_show(){};

    virtual void on_hide(){};

    virtual void on_focus(){};

    virtual void on_blur(){};

    virtual bool on_touch(const st_inputEvent) {
        return false;
    };

    virtual Menu::menu_actions_st *get_quick_actions() {
        return nullptr;
    };

    virtual bool on_input(const st_inputEvent event);

    virtual const std::vector<Widget *> &children() const;

    // State management methods.
    virtual void set_dirty();

    void set_bg(Color c);

    Color get_bg();

    uint16_t get_fg() const;

    void set_fg(uint16_t fg);

    bool dirty() const;

    bool set_focus(bool value);

    bool is_focused() const;

    void set_clean();

    Display *get_display() const;

    void set_display(Display *display);

    bool can_be_seen();

    bool is_point_visible(Point &p);

    bool visible();

    void set_visible(bool v);

    bool active();

    void set_active(bool v);

    bool enabled();

    void set_enabled(bool v);

    uint16_t get_z_index() const;

    void set_z_index(uint16_t z_index);

    virtual void set_font(const FontDef *);

    FontDef const *get_font() {
        return font;
    };

    bool focusable() {
        return flags.focusable;
    }

    void set_focusable(bool v) {
        flags.focusable = v;
    }

    uint32_t id{0};

    void set_aling(Align);

    void set_name(const char *);

    char *get_name();

    Rect clip(const Rect &rect);
    // Vector of visible rectangles. There are no overlaps if empty
    std::vector<Rect> visible_rects;

  protected:
    char name[5]{"-"};

    Rect _parent_rect;

    uint16_t z_index = 0;

    Display *display;

    Area area;

    Widget *parent_{nullptr};

    FontDef const *font = (FontDef *)&Font_Tiny8x8;

    Color bg_color{C565_BLACK};
    uint16_t fg_color{C565_GREY_LIGHT};
    Align align = ALIGN_LEFT;

    // FPS measurement
    float fps;

    bool show_fps{false};

    uint64_t last_refresh_ms;

    struct flags_t {
        bool dirty : 1;   // Widget content has changed.
        bool hidden : 1;  // Object was hidden during last refresh.
        bool visible : 1; // Paint the widget or not?
        bool focus : 1;   // Widget has focus
        bool focusable : 1;
        bool active : 1;
        bool enabled : 1;
    };

    flags_t flags{.dirty = true, .hidden = false, .visible = true, .focus = false, .focusable = false, .active = false, .enabled = true};

    static const std::vector<Widget *> no_children;

    virtual void widget_focused(Widget *widget);

    virtual void set_area();

    virtual void on_child_update(Widget *){};

    void refresh_fps();

    void paint_overlapped();

    Box getOffset(Rect &r, Box &offset, bool apply_pad);

    bool paint_callback() override {
        // DEBUG FOCUS
        if (focusable()) {
            display->writeRect(0, 0, area.box.width - 1, area.box.height - 1, is_focused() ? C565_YELLOW : C565_CYAN);
        }
        return true;
    }

    friend class View;
};

class HasPadding {
  public:
    // Set padding
    void set_padding(uint16_t x, uint16_t y) {
        padding_x = x;
        padding_y = y;
    }

    // Get padding values
    uint16_t get_padding_x() const {
        return padding_x;
    }
    uint16_t get_padding_y() const {
        return padding_y;
    }

  protected:
    uint16_t padding_x{0};
    uint16_t padding_y{0};
};

#endif /*__WIDGET_H__*/
