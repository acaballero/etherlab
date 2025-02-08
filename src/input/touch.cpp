//
// Created by Angel Dust on 09/07/2024.
//

#include "touch.h"
#include "os/periodic_task.h"
#include "XPT2046_touch.h"
#include "input.h"

namespace touch {

void check_touch();

os::periodic_task task(100, check_touch);

void check_touch() { TouchPanelInterruptPin.checkState(); }

} // namespace touch
