//
// Created by Angel Dust on 19/05/2025.
//

#include <cfloat>
#include <stdint.h>
#include "Display_afb.h"
#include "console_widget.h"
#include "../config.h"
#include "ips_font.h"

ConsoleWidget::ConsoleWidget(Rect parent_rect, Display *display) : Widget(parent_rect, display) {

    line_buffer.fill("");
}

void ConsoleWidget::paint_callback() {
    display->clear();
    display->setFont(this->font);
    display->setBgColor(C565_BLACK);

    bool escape = false;

    uint16_t color = C565_WHITE;

    uint16_t y = 0;
    for (size_t i = 0; i < line_count; ++i) {
        size_t idx = (line_head + i) % rows;
        const std::string &line = line_buffer[idx];

        display->gotoXY(0, y);

        for (char c : line) {
            if (escape) {
                color = (c < 16) ? palette16[(uint8_t)c] : C565_WHITE;
                escape = false;
            } else if (c == '\x1B') {
                escape = true;
            } else {
                display->setColor(color);
                display->writeChar(c);
            }
        }

        y += font->height + display->getVerticalLineSpacing();
    }
}

void ConsoleWidget::before_paint() {
    uint64_t m = HAL_GetTick();
    if (m - this->last_refresh_ms < this->update_period_ms && this->dirty()) {
        this->set_clean();
    } else {
        this->cols = area.box.width / font->width;
        this->rows = min2(area.box.height / (font->height + display->getVerticalLineSpacing()), max_lines);
    }
}

void ConsoleWidget::write(std::string message) {

    size_t pos = 0;
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

        line_buffer[line_head] = line;
        line_head = (line_head + 1) % rows;
        if (line_count < rows) {
            line_count++;
        }
    }

    set_dirty();
}

void ConsoleWidget::writeln(std::string message) {
    write(message + "\n");
}
