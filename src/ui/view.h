#ifndef _DISPLAY_H
#define _DISPLAY_H

#include "ui_types.h"
#include "widget.h"
#include "lcd.h"

class View : public Widget {

  public:
    View() : Widget({0, 0, DISPLAY_X_PIXELS, DISPLAY_Y_PIXELS}, &lcd) {}

    View(Rect parent_rect) : Widget(parent_rect, &lcd) {}

    void set_parent_rect(Rect) override;

    void add_child(Widget *const widget);

    void add_children(const std::initializer_list<Widget *> children);

    void remove_child(Widget *const widget);

    const std::vector<Widget *> &children() const override;

    bool on_input(const st_inputEvent event) override;

    void on_hide() override;

    void (*on_hide_fn)(void){};

    void paint() final;

  protected:
    std::vector<Widget *> children_{};

    // void invalidate_child(Widget *const widget);

    // Those methods are no longer public

    void paint_callback() final;

    void set_area() override;
};

#endif
