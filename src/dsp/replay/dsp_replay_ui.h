//
// Created by Angel Dust on 16/04/2021.
//

#ifndef TRX_FRONTEND_DSP_REPLAY_UI_H
#define TRX_FRONTEND_DSP_REPLAY_UI_H

#include "../../../lib/Menu/src/menu.h"
#include "../dsp_common.h"

namespace dspReplayUI {
extern Menu::menu replayMenu;

void on_event(st_dsp_params *status);
Menu::result change_dsp_status(Menu::eventMask e);
Menu::result on_menu_event(Menu::eventMask e);
Menu::result on_filepicker(Menu::eventMask e);
} // namespace dspReplayUI

#endif // TRX_FRONTEND_DSP_REPLAY_UI_H
