//
// Created by Angel Dust on 19/05/2025.
//

#include <cfloat>
#include <stdint.h>
#include <string>
#include <algorithm>
#include "Display_afb.h"
#include "console_widget.h"
#include "../config.h"
#include "ips_font.h"
#include "os/periodic_task.h"

ConsoleWidget::ConsoleWidget(Rect parent_rect, Display *display) : Widget(parent_rect, display) {
    clear();
}

bool ConsoleWidget::paint_callback() {

    display->setBgColor(get_bg());
    display->fillBuffer(get_bg());
    display->setFont(font);

    bool escape = false;

    uint16_t color = C565_WHITE;

    uint16_t y = padding_y + (rows - line_count) * line_height + 1;
    for (size_t i = 0; i < line_count; ++i) {
        size_t idx = (line_head - line_count + i + rows) % rows; // go back with modulus
        const std::string &line = line_buffer[idx];

        display->gotoXY(padding_x, y);

        for (char c : line) {
            if (escape) {
                // Color codes start at 1 but array indexes at 0
                color = (c <= 16) ? palette16[((uint8_t)c) - 1] : C565_WHITE;
                escape = false;
            } else if (c == color_mark) {
                escape = true;
            } else {
                display->setColor(color);
                display->writeChar(c);
            }
        }

        y += line_height;
    }

    return true;
}

void ConsoleWidget::before_paint() {
    uint64_t m = HAL_GetTick();
    if (m - last_refresh_ms < update_period_ms && dirty()) {
        this->set_clean();
    } else if (dirty()) {
        display->setVerticalLineSpacing(line_spacing);
        calc_size();
    }
}

void ConsoleWidget::calc_size() {
    line_height = font->height + line_spacing;
    cols = (area.box.width - 2 * padding_x) / font->width;

    size_t curr_rows = rows;
    rows = min2((area.box.height - (padding_y * 2)) / line_height, max_lines);

    int new_lines = rows - curr_rows;
    if (new_lines > 0) { // make space
        for (int i = 0; i < new_lines; i++) {
            line_buffer[line_head + i + new_lines] = line_buffer[line_head + i];
        }
    }
    // TODO: Consider when shrinking
}

void ConsoleWidget::set_rows(size_t r) {
    set_height(line_height * r + (padding_y * 2));
}

void ConsoleWidget::set_parent_rect(Rect r) {
    Widget::set_parent_rect(r);
    calc_size();
}

void ConsoleWidget::clear() {
    line_buffer.fill("");
    line_count = 0;
    line_head = 0;
    live_line_present = false;
}

void ConsoleWidget::set_live_line(const std::string &line) {
    if (rows == 0) {
        return;
    }

    if (!live_line_present) {
        line_buffer[line_head] = line;
        line_head = (line_head + 1) % rows;
        if (line_count < rows) {
            line_count++;
        }
        live_line_present = true;
    } else {
        size_t idx = (line_head + rows - 1) % rows;
        line_buffer[idx] = line;
    }

    set_dirty();
}

void ConsoleWidget::commit_live_line() {
    live_line_present = false;
}

void ConsoleWidget::write(const std::string &message) {

    // Explicit line writes start committed line mode.
    live_line_present = false;

    size_t pos = 0;
    int next_color = -1;
    int last_line = -1;

    while (pos < message.size()) {
        std::string line;
        size_t count = 0;
        while (count < cols && pos < message.size()) {
            char c = message[pos++];
            line += c;
            if (c == '\n') {
                break;
            }
            count++;
        }

        // Strip trailing newline
        if (!line.empty() && line.back() == '\n') {
            line.pop_back();
        }

        // If color codes will be overwritten, move the last one to the end so the first visible lines keep their color

        std::string curr_line = line_buffer[line_head];

        size_t pos = curr_line.rfind(static_cast<char>(color_mark));
        if (pos != std::string::npos && pos + 1 < curr_line.size()) {
            next_color = curr_line[pos + 1];
        }

        // Trim
        line.erase(line.begin(), std::find_if_not(line.begin(), line.end(), ::isspace));

        line_buffer[line_head] = line;
        last_line = line_head;
        line_head = (line_head + 1) % rows;

        if (line_count < rows) {
            // Count until lines reach max available rows
            line_count++;
        }
    }

    if (next_color != -1) {
        line_buffer[last_line] = color_mark + std::string(1, next_color) + line_buffer[last_line];
    }

    set_dirty();
}

void ConsoleWidget::writeln(const std::string &message) {
    write(message + "\n");
}
