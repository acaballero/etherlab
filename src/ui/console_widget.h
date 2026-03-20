//
// Created by Angel Dust on 19/05/2025.
//

#ifndef CONSOLE_WIDGET_H
#define CONSOLE_WIDGET_H

#include "widget.h"
#include "types.h"
#include <sys/_stdint.h>

class ConsoleWidget : public Widget, public HasPadding {
  public:
    ConsoleWidget(Rect parent_rect, Display *display);
    bool paint_callback() override;

    void clear();
    void write(const std::string &message);
    void writeln(const std::string &message);
    void set_live_line(const std::string &line);
    void commit_live_line();
    void set_parent_rect(Rect) override;
    void set_rows(size_t r);
    uint32_t get_line_count() {
        return line_count;
    };

    static constexpr char color_mark = '\x1B';

  protected:
    uint32_t update_period_ms = 200;
    static constexpr size_t max_lines = 20; // set to max possible rows (adjust if needed)

    size_t rows = 0;
    size_t cols = 0;
    uint16_t line_height = 0;
    uint16_t line_spacing = 2;
    std::array<std::string, max_lines> line_buffer;
    size_t line_head = 0;
    size_t line_count = 0;
    bool live_line_present = false;

    void wrap(const std::string &raw_line);
    void calc_size();
    void before_paint() override;
};

#endif // CONSOLE_WIDGET_H
