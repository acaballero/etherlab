//
// Created by Angel Dust on 16/04/2021.
//

#ifndef TRX_FRONTEND_DSP_UI_H
#define TRX_FRONTEND_DSP_UI_H

#include "hw/stm32.h"
#include <stdio.h>
#include "items.h"
#include "menu.h"
#include "dsp/signal_generator/dsp_signal_generator_ui.h"
#include "dsp/replay/dsp_replay_ui.h"
#include "dsp/capture/dsp_capture_ui.h"
#include "menuBase.h"

namespace dsp_ui {

extern Menu::menu menuDSP;
extern bool dsp_enabled;
void refresh_menu_state();
Menu::result apply_dsp_changes(Menu::eventMask = Menu::noEvent);

} // namespace dsp_ui

#endif // TRX_FRONTEND_DSP_UI_H
