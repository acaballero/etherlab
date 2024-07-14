//
// Created by Angel Dust on 12/10/2022.
//
#include "sd_filepicker_menu.h"
#include "status.h"

Menu::result delete_file(Menu::eventMask e) {

    char path[PATH_SIZE];

    strncpy(path, filePicker.focusedFolder, PATH_SIZE-1);
    strncat(path, filePicker.focusedFile, PATH_SIZE-1);

    FRESULT res = f_unlink(path);

    if (res == FR_OK) {
        filePicker.refresh();
    } else {
        // Throw error
        status::handleError(status::ST_ERROR, "Error deleting file");
    }

    return Menu::quit;
}

using namespace Menu;

prompt *subData[] = {
        new prompt("Delete", delete_file, enterEvent),
        new Exit("<Back")
};

menuNode fileSubmenu = menuNode("Options", sizeof(subData) / sizeof(prompt *), subData);

SDMenuT filePicker("File", "", doNothing, (eventMask) (updateEvent | enterEvent | refreshEvent));
