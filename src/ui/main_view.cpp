//
// Created by Angel Dust on 17/04/2021.
//
#include "power_amp.h"
#include "rf_coupler.h"
#include "main_view.h"
#include "lcd.h"
#include "menu.h"
#include "status.h"
#include "view_manager.h"

MainView::MainView() : View({0, 0, DISPLAY_X_PIXELS + DISPLAY_PADDING * 2, DISPLAY_Y_PIXELS + DISPLAY_PADDING * 2}) {

   this->fft_w.set_show_fps(true);
   this->tune_w.set_visible(config.debug);
   this->radio_w.set_visible(!config.debug);
   this->info_w.set_visible(config.debug);
   this->info_w.set_show_fps(config.debug);
   this->menu_w.set_show_fps(config.debug);
   this->smeter_w.set_visible(false);
   this->powmeter_w.set_visible(false);
   this->msg_w.set_z_index(10);
   this->msg_w.set_visible(false);
   this->msg_w.get_display()->setPadding(8, 8);
   this->msg_w.get_display()->setVerticalLineSpacing(1);

   this->children_.reserve(40);

   add_children({&this->menu_w, &this->header_w, &this->tune_w, &this->smeter_w, &this->radio_w, &this->powmeter_w, &this->info_w, &this->status_w,
                 &this->dbscale_w, &this->frequency_w, &this->iqbal_w, &this->waterfall_w, &this->fft_w, &this->msg_w});
}

void MainView::do_paint() {

   if (msg_w.visible()) {
      tune_w.set_visible(false);
      info_w.set_visible(false);
      smeter_w.set_visible(false);
      powmeter_w.set_visible(false);
      radio_w.set_visible(false);
      menu_w.set_visible(false);
   } else {
      if (menuStatus == IDLE) {
         if (config.debug) {
            smeter_w.set_visible(false);
            radio_w.set_visible(true);
            powmeter_w.set_visible(false);
            tune_w.set_visible(true);
            info_w.set_visible(true);
         } else {
            smeter_w.set_visible(!ISTX);
            radio_w.set_visible(true);
            powmeter_w.set_visible(ISTX);
            tune_w.set_visible(false);
            info_w.set_visible(false);
         }
         menu_w.set_visible(false);
      } else {
         tune_w.set_visible(false);
         radio_w.set_visible(false);
         smeter_w.set_visible(false);
         powmeter_w.set_visible(false);
         info_w.set_visible(false);
         menu_w.set_visible(true);
      }
   }
}

WaterfallWidget *MainView::Waterfall() { return &this->waterfall_w; }

Widget *MainView::Spectrum() { return &this->fft_w; }

Widget *MainView::IQBalance() { return &this->iqbal_w; }

Widget *MainView::TuneInfo() { return &this->tune_w; }

Widget *MainView::FFTInfo() { return &this->info_w; }

Widget *MainView::FFT() { return &this->fft_w; }

Widget *MainView::Menu() { return &this->menu_w; }

Widget *MainView::Message() { return &this->msg_w; }

bool MainView::on_input(const st_inputEvent event) {

   bool consumed;

   if (!focused_widget()) {
      menu_w.set_focus(true);
   }

   consumed = View::on_input(event);

   if (!consumed) {
   }

   return consumed;
}
