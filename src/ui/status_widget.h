//
// Created by Angel Dust on 17/04/2021.
//

#ifndef TRX_FRONTEND_STATUS_WIDGET_H
#define TRX_FRONTEND_STATUS_WIDGET_H

#include "view.h"
#include "button_widget.h"
#include "../types.h"

#define BTN_WIDTH (DISPLAY_X_PIXELS / 6)
class StatusWidget : public View {
 public:
   StatusWidget(Rect parent_rect) : View(parent_rect) { init(); }
   void paint_callback() override;

 protected:
   st_status _status;

   uint16_t fg_color, fg_color_auto, bg_color, dimm_color, disabled_color, disabled_bg;

   Button btnModulation{{0, 0, BTN_WIDTH - 1, area.height}, display, "", C565_BLACK};
   Button btnFrontend{{BTN_WIDTH, 0, BTN_WIDTH - 1, area.height}, display, "", C565_BLACK};
   Button btnAgc{{BTN_WIDTH * 2, 0, BTN_WIDTH - 1, area.height}, display, "", C565_BLACK};
   Button btnBand{{BTN_WIDTH * 3, 0, BTN_WIDTH - 1, area.height}, display, "", C565_BLACK};
   Button btnFilter1{{BTN_WIDTH * 4, 0, BTN_WIDTH - 1, area.height}, display, "", C565_BLACK};
   Button btnFilter2{{BTN_WIDTH * 5, 0, BTN_WIDTH, area.height}, display, "", C565_BLACK};

   char buf[20];

   void do_paint() override;

   void init();

   void mode();

   const char *modulation();

   void band(Widget *);

   void filter1(Widget *);

   void filter2(Widget *);

   char *frontend();

   char *agc_alc();
};

#endif // TRX_FRONTEND_STATUS_WIDGET_H
