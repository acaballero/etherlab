#include "receive_ui.h"

#include "config.h"
#include "cw_decoder.h"
#include "dsp/dsp_common.h"
#include "main_board.h"
#include "status.h"
#include "ui/console_widget.h"
#include "dsp/fft/waterfall_widget.h"
#include "dsp/fft/iqbalance_widget.h"

#include <cstdint>
#include <string>

namespace dspReceiveUI {
namespace {
static constexpr size_t CW_CONSOLE_WRAP_COLS = 44;
static constexpr uint32_t CW_LINE_BREAK_IDLE_MS = 1200;

ConsoleWidget *g_console = nullptr;
WaterfallWidget *g_waterfall = nullptr;
IQBalanceWidget *g_iqbalance = nullptr;

Rect g_waterfall_rect{};
Rect g_iqbalance_rect{};

bool g_layout_active = false;
SignalToken g_decode_signal_token{0};
std::string g_pending_line;
size_t g_pending_visible_len = 0;
bool g_pending_escape = false;
uint32_t g_last_char_ms = 0;



void flush_line(bool force_newline) {
    if (!g_console) {
        g_pending_line.clear();
        return;
    }

    if (g_pending_line.empty()) {
        if (force_newline) {
            g_console->commit_live_line();
        }
        return;
    }

    g_console->set_live_line(g_pending_line);
    if (force_newline) {
        g_console->commit_live_line();
    }

    g_pending_line.clear();
    g_pending_visible_len = 0;
    g_pending_escape = false;
}

void apply_layout(bool enabled) {
    if (!g_console || !g_waterfall || !g_iqbalance) {
        return;
    }

    if (enabled == g_layout_active) {
        return;
    }

    g_layout_active = enabled;

    if (enabled) {
        const uint16_t half_h = g_waterfall_rect.height() / 2;
        g_waterfall->set_parent_rect({g_waterfall_rect.left(), g_waterfall_rect.top(), g_waterfall_rect.width(), half_h});
        g_iqbalance->set_parent_rect({g_iqbalance_rect.left(), g_iqbalance_rect.top(), g_iqbalance_rect.width(), half_h});
        g_console->set_parent_rect({g_waterfall_rect.left(), g_waterfall_rect.top() + half_h, g_waterfall_rect.width(), g_waterfall_rect.height() - half_h});
        g_pending_line.clear();
        g_pending_visible_len = 0;
        g_pending_escape = false;
        g_console->clear();
        g_console->set_visible(true);
    } else {
        flush_line(true);
        g_waterfall->set_parent_rect(g_waterfall_rect);
        g_iqbalance->set_parent_rect(g_iqbalance_rect);
        g_pending_line.clear();
        g_pending_visible_len = 0;
        g_pending_escape = false;
        g_console->clear();
        g_console->set_visible(false);
    }
}

void on_decoded_text(const cw_decode::text_event *event) {
    if (!event || event->text[0] == '\0') {
        return;
    }

    if (!(g_layout_active && g_console && g_console->visible())) {
        return;
    }

    for (const char *p = event->text; *p; ++p) {
        const char c = *p;

        if (c == '\n' || c == '\r') {
            g_last_char_ms = HAL_GetTick();
            flush_line(true);
            continue;
        }

        g_pending_line.push_back(c);
        g_last_char_ms = HAL_GetTick();

        if (g_pending_escape) {
            g_pending_escape = false;
        } else if (c == ConsoleWidget::color_mark) {
            g_pending_escape = true;
        } else {
            g_pending_visible_len++;
        }

        if (g_pending_visible_len >= CW_CONSOLE_WRAP_COLS) {
            flush_line(true);
        }

        if (!g_pending_line.empty()) {
            g_console->set_live_line(g_pending_line);
        }
    }
}

} // namespace

void deinit() {
    if (g_decode_signal_token) {
        cw_decode::text_signal.remove(g_decode_signal_token);
        g_decode_signal_token = 0;
    }

    g_layout_active = false;
    g_pending_line.clear();
    g_pending_visible_len = 0;
    g_pending_escape = false;
    g_last_char_ms = 0;

    g_console = nullptr;
    g_waterfall = nullptr;
    g_iqbalance = nullptr;
}

void init(ConsoleWidget *console, WaterfallWidget *waterfall, IQBalanceWidget *iqbalance) {
    deinit();

    g_console = console;
    g_waterfall = waterfall;
    g_iqbalance = iqbalance;

    if (g_waterfall) {
        g_waterfall_rect = g_waterfall->parent_rect();
    }
    if (g_iqbalance) {
        g_iqbalance_rect = g_iqbalance->parent_rect();
    }

    if (g_console) {
        g_console->set_visible(false);
        g_console->clear();
    }

    g_decode_signal_token = cw_decode::text_signal.add(nullptr, [](void *, const void *params) {
        on_decoded_text(static_cast<const cw_decode::text_event *>(params));
    });
}

void before_paint() {
    const bool show = dsp::dsp_config.decode_cw && (main_board::get_modulation_mode() == CW);
    apply_layout(show);

    if (show && !g_pending_line.empty()) {
        const uint32_t now = HAL_GetTick();
        if ((now - g_last_char_ms) >= CW_LINE_BREAK_IDLE_MS) {
            flush_line(true);
        }
    }
}

} // namespace dspReceiveUI
