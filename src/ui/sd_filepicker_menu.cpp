//
// Created by Angel Dust on 12/10/2022.
//
#include "sd_filepicker_menu.h"
#include "status.h"

using namespace Menu;

SDMenuT filePicker("File", "", doNothing, (eventMask)(updateEvent | enterEvent | refreshEvent));
