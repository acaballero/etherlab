
#ifndef __WIDGET_H__
#define __WIDGET_H__

#include "ui_types.h"
#include "../input/inputEvent.h"
#include "../../lib/ST77XX-STM32/Display_afb.h"
#include "../../lib/ST77XX-STM32/Painter.hpp"
#include "../../lib/printf/printf.h"
#include "lcd.h"
#include <vector>


enum Align {
    ALIGN_LEFT,
    ALIGN_RIGHT,
    ALIGN_CENTER
};

class Widget : public Painter {
public:

    Widget() : _parent_rect{}, display{&lcd} {}

    Widget(Rect parent_rect, Display *display) : _parent_rect{parent_rect}, display{display} {
        this->set_area();
    }

    Widget(const Widget &) = delete;

    Widget(Widget &&) = delete;

    Widget &operator=(const Widget &) = delete;

    Widget &operator=(Widget &&) = delete;

    virtual ~Widget() = default;

    Point screen_pos();

    Size size() const;

    Rect screen_rect() const;

    Rect parent_rect() const;

    void set_show_fps(bool);

    virtual void set_parent_rect(const Rect new_parent_rect);

    Widget *parent() const;

    Widget *focused_widget() const;

    void set_parent(Widget *const widget);

    bool hidden() const { return flags.hidden; }

    void hidden(bool hide);

    void paint();

    virtual void on_show() {};

    virtual void on_hide() {};

    virtual void on_focus() {};

    virtual bool on_input(const st_inputEvent event);

    virtual const std::vector<Widget *> &children() const;

    virtual // State management methods.
    void set_dirty();

    bool dirty() const;

    void set_focus(bool value);

    bool is_focused() const;

    void set_clean();

    Display *get_display() const;

    void set_display(Display *display);

    bool visible();

    void set_visible(bool v);

    bool active();

    void set_active(bool v);

    bool enabled();

    void set_enabled(bool v);

    uint8_t get_z_index() const;

    void set_z_index(uint8_t z_index);

    void set_font(FontDef *);

    uint32_t id = 0;

    void set_aling(Align);

protected:
    void dirty_overlapping_children_in_rect(const Rect &child_rect);

    Rect _parent_rect;

    uint8_t z_index = 0;

    Display *display;

    Area area;

    Widget *parent_{nullptr};

    FontDef *font = (FontDef *) &Font_Tiny8x8;

    Align align = ALIGN_LEFT;

    // FPS measurement
    float fps;
    bool show_fps;
    uint64_t last_refresh_ms;

    struct flags_t {
        bool dirty: 1;          // Widget content has changed.
        bool hidden: 1;         // Object was hidden during last refresh.
        bool visible: 1;        // Paint the widget or not?
        bool focus: 1;          // Widget has focus
        bool active: 1;
        bool enabled: 1;
    };

    flags_t flags{
            .dirty = true,
            .hidden = false,
            .visible = true,
            .focus = false,
            .active = false,
            .enabled = true
    };

    static const std::vector<Widget *> no_children;

    void focus(Widget *widget);

    void set_area();

    void update_overlaps();

    virtual void do_paint() = 0;
};


#endif/*__WIDGET_H__*/
