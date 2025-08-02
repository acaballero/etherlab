//
// Created by Angel Dust on 19/05/2025.
//

#ifndef CONSOLE_WIDGET_H
#define CONSOLE_WIDGET_H

#include "widget.h"
#include "types.h"

class ConsoleWidget : public Widget {
  public:
    ConsoleWidget(Rect parent_rect, Display *display);
    bool paint_callback() override;

    void write(const std::string &message);
    void writeln(const std::string &message);

    static constexpr char color_mark = '\x1B';

  protected:
    uint32_t update_period_ms = 200;
    static constexpr size_t max_lines = 20; // set to max possible rows (adjust if needed)

    size_t rows = 0;
    size_t cols = 0;

    std::array<std::string, max_lines> line_buffer;
    size_t line_head = 0;
    size_t line_count = 0;
    void wrap(const std::string &raw_line);

    void before_paint() override;
};

#endif // CONSOLE_WIDGET_H
