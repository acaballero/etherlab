//
// Created by Angel Dust on 17/06/2025.
//

#ifndef TRX_FRONTEND_LOCK_VIEW_H
#define TRX_FRONTEND_LOCK_VIEW_H

#include "Display_afb.h"
#include "input/inputEvent.h"
#include "stdio.h"
#include "text_widget.h"
#include "view.h"
#include "label_widget.h"
#include "main_view.h"

class LockView : public View {
  public:
    static constexpr uint16_t HEIGHT = DISPLAY_Y_PIXELS + (DISPLAY_PADDING * 2);
    static constexpr uint16_t WIDTH = DISPLAY_X_PIXELS + (DISPLAY_PADDING * 2);
    static constexpr uint16_t TOP = 100;

    LockView() : View({0, 0, WIDTH, HEIGHT}) {
        init();
    }

    void init();

    void before_paint() override;
    bool on_input(const st_inputEvent) override;

  private:
    Label lblTitle{{30, TOP + 0, DISPLAY_X_PIXELS - 60, 30}};
    TextWidget lblText1{{30, TOP + 40, DISPLAY_X_PIXELS - 60, 30}, ""};
};

#endif // TRX_FRONTEND_LOCK_VIEW_H
