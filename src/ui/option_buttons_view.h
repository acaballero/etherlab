//
// Created by Angel Dust on 12/07/2024.
//

#ifndef TRX_FRONTEND_OPTION_BUTTONS_VIEW_H
#define TRX_FRONTEND_OPTION_BUTTONS_VIEW_H

#include "Display_afb.h"
#include "display_panel_buttons_widget.h"
#include "menu_options.h"
#include "stdio.h"
#include "ui/widget.h"
#include "view.h"
#include "button_widget.h"
#include "math.h"
#include "label_widget.h"
#include <stdint.h>

class OptionButtonsView : public View {
  public:
    static constexpr uint16_t TITLE_HEIGHT = 22;
    static constexpr uint16_t HEIGHT = DISPLAY_Y_PIXELS - TITLE_HEIGHT;
    static constexpr uint16_t WIDTH = DISPLAY_X_PIXELS;
    static constexpr uint8_t MAX_BUTTONS = 32;

    OptionButtonsView() : View() { init(); }

    OptionButtonsView(Rect parent_rect) : View(parent_rect) { init(); }

    void on_focus() override;

    bool on_input(const st_inputEvent event) override;

    std::function<void(uint16_t)> on_select;

    void init();

    void before_paint() override;

    void set_title(const char *text);

    void add_item(const char *text, std::function<void(Button &, st_inputEvent)> on_select, bool selected, uint16_t fg_color = C565_TEXT_FG,
                  uint16_t text_bg_color = C565_TRANSPARENT);

    void clear();

    Button get_item(uint16_t index);

    void set_show_arrows(bool b);

  private:
    int focused_button = -1;
    static constexpr int max_cols = 4;
    static constexpr int max_rows = 4;

    int button_w = WIDTH / max_cols;
    int button_h = (HEIGHT - TITLE_HEIGHT) / max_rows;

    Button buttons[MAX_BUTTONS];
    uint8_t index = 0;
    uint16_t offset = -1;
    uint16_t cols = max_cols;
    uint16_t rows = max_rows;
    bool show_arrows = true;

    Button button_prev{
        {button_w * (cols - 1), button_h *rows - 1, button_w, button_h}, display, "<", C565_BLACK, C565_GREY_DARK, BUTTON_STYLE_3D, ALIGN_CENTER};
    Button button_next{
        {button_w * (cols - 1), button_h *rows - 1, button_w, button_h}, display, ">", C565_BLACK, C565_GREY_DARK, BUTTON_STYLE_3D, ALIGN_CENTER};
    Button button_close{
        {button_w * (cols - 1), button_h *rows - 1, button_w, button_h}, display, "Cancel", C565_WHITE, C565_GREY_DARKER, BUTTON_STYLE_3D, ALIGN_CENTER};

    Label label_widget{{0, 0, WIDTH, TITLE_HEIGHT}, C565_WHITE, C565_GREY_DARKER, ButtonStyle::BUTTON_STYLE_FLAT};

    const char *display_buttons_labels[6] = {"<", ">", "", "", "", "Cancel"};
    DisplayPanelButtonsWidget display_panel_buttons = {{0, HEIGHT - TITLE_HEIGHT, WIDTH, TITLE_HEIGHT}};

    bool update_focus(int button_index);
    void update_buttons(bool forze);
};

#endif // TRX_FRONTEND_OPTION_BUTTONS_VIEW_H
