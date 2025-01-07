//
// Created by Angel Dust on 07/01/2025.
//

#ifndef TRX_FRONTEND_SPLASH_VIEW_H
#define TRX_FRONTEND_SPLASH_VIEW_H

#include "Display_afb.h"
#include "stdio.h"
#include "view.h"
#include "label_widget.h"
#include "main_view.h"

class SplashView : public View {
  public:
    static constexpr uint16_t HEIGHT = DISPLAY_Y_PIXELS + (DISPLAY_PADDING * 2);
    static constexpr uint16_t WIDTH = DISPLAY_X_PIXELS + (DISPLAY_PADDING * 2);
    static constexpr uint16_t TOP = 100;

    SplashView() : View({0, 0, WIDTH, HEIGHT}) { init(); }

    void init();

    void do_paint() override;

  private:
    Label lblTitle{{30, TOP + 0, WIDTH - 60, 30}};
    Label lblText1{{30, TOP + 40, WIDTH - 60, 10}};
    Label lblText2{{30, TOP + 55, WIDTH - 60, 10}};
    Label lblText3{{30, TOP + 70, WIDTH - 60, 10}};
};

#endif // TRX_FRONTEND_SPLASH_VIEW_H
