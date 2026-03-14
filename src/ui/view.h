#ifndef _DISPLAY_H
#define _DISPLAY_H

#include "label_widget.h"
#include "widget.h"

#include <functional>
#include <initializer_list>
#include <vector>

class View : public Widget {

  public:
    View() : View({0, 0, DISPLAY_X_PIXELS, DISPLAY_Y_PIXELS}) {
    }

    View(Rect parent_rect, const char *title = nullptr) : Widget(parent_rect, Widget::default_display()) {

        if (title) {

            title_w.set_parent_rect({3, 3, parent_rect.width() - 6, 20});
            title_w.set_border_radius(false, false, false, false);
            title_w.set_aling(Align::ALIGN_CENTER);
            title_w.set_label(title);
            set_border_width(1);
            set_border_color(C565_GREY_DARK);
            set_shadow_width(2);
            add_child(&title_w);
        }
    }

    View(View &&) = delete;

    void set_parent_rect(Rect) override;

    void add_child(Widget *const widget);

    void to_top(Widget *widget);

    void add_children(const std::initializer_list<Widget *> children);

    bool remove_child(Widget *const widget);

    const std::vector<Widget *> &children() const override;

    bool on_input(const st_inputEvent event) override;

    void on_hide() override;

    std::function<void()> on_hide_fn;

    void paint(Area *area = nullptr) final;

    void set_border_width(uint16_t w) {
        border_width = w;
    }

    void set_shadow_width(uint16_t w) {
        shadow_width = w;
    }

    int16_t get_border_width() {
        return border_width;
    }

    int16_t get_shadow_width() {
        return shadow_width;
    }

    void set_border_color(Color c) {
        border_color = c;
    }

  protected:
    std::vector<Widget *> children_{};

    // Vector of overlapping widgets (maintained for performace at the cost of memory)
    // std::map<Widget *, std::vector<Widget *>> overlap_map;

    void on_child_update(Widget *) override;

    void set_area() override;

    bool paint_callback() final; // Note this is no longer overridable

  private:
    uint16_t border_width{0};
    uint16_t shadow_width{0};
    Color border_color{C565_GREY_LIGHT};

    Label title_w{{}, C565_WHITE, C565_GREY_DARKER, ButtonStyle::BUTTON_STYLE_FLAT};
};

#endif
