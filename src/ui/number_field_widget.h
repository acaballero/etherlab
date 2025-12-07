//
// Created by Angel Dust on 16/06/2025.
//

#ifndef NUMBER_FIELD_WIDGET_H
#define NUMBER_FIELD_WIDGET_H

#include "widget.h"
#include <cstdint>

class NumberField : public Widget {
  public:
    std::function<void(NumberField &)> on_select{};
    std::function<void(int32_t)> on_change{};
    std::function<void(int32_t)> on_wrap{};

    using range_t = std::pair<int32_t, int32_t>;

    NumberField(Point parent_pos, int length, range_t range, int32_t step, const char *u, bool can_loop);

    NumberField(Point parent_pos, int length, range_t range, int32_t step, const char *u) : NumberField{parent_pos, length, range, step, u, false} {
    }

    NumberField() : NumberField{{0, 0}, 1, {0, 1}, 1, "", false} {
    }

    NumberField(const NumberField &) = delete;
    NumberField(NumberField &&) = delete;

    int32_t get_value() const;
    void set_value(int32_t new_value, bool trigger_change = true);
    void set_range(const int32_t min, const int32_t max);
    void set_step(const int32_t new_step);

    void add(int32_t v);
    bool paint_callback() override;
    void before_paint() override;
    bool on_input(const st_inputEvent event) override;

    void set_font(const FontDef *f) override;

  private:
    static constexpr int max_length = 14;
    range_t range;
    int32_t step;
    const int length;
    char units[6] = "-";
    int32_t value{INT32_MAX};
    char text[max_length];
    bool can_loop{};
    void calc_size();
};

#endif
