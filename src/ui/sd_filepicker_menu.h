#ifndef TRX_FRONTEND_SD_FILEPICKER_MENU_H
#define TRX_FRONTEND_SD_FILEPICKER_MENU_H

#include <status.h>
#include "hw/stm32.h"
#include "ui/menu.h"
#include "../../lib/Menu/src/menu.h"
#include "../../lib/FatFs/ff.h"
#include "../fatfs/fatfs.h"
#include "../../lib/utils/utils.hpp"

extern Menu::menuNode fileSubmenu;

// TODO: Error handling

// SD Card driver
// We avoid allocating memory here, instead we read all info from SD
class FSO {

public:

    DIR dir;
    FIL *file;
    FILINFO fileinfo;
    // In FatFS v0.11 we have to allocate a buffer if we're to use the long file name
    TCHAR lfname[FN_SIZE + 1];
    // FILINFO finfocache[10];
    int16_t curr_ix = -1;
    int16_t curr_folder_count = -1;

    FSO() {
        file = &FatFSFileHandle;
        // Assign the long filename memory
        fileinfo.lfname = lfname;
        fileinfo.lfsize = FN_SIZE;
    }

    virtual ~FSO() { f_closedir(&dir); }

    FRESULT openFolder(const char *path) {
        FRESULT fres = f_opendir(&dir, path);
        curr_folder_count = -1; // reset folder count
        curr_ix = -1;
        return fres;
    }

    // Go to a file with index 'ix' in the current dir or to the last file if 'ix' is negative
    // Returns the stop index
    // The file info gets stored in the 'fileinfo' member
    long gotoIndex(int16_t ix) {

        FRESULT fres;

        if (ix == curr_ix && ix > -1) {
            return curr_ix;
        }

        if (ix <= curr_ix || curr_ix == -1) {
            fres = f_rewinddir(&dir);
            if (fres == FR_OK) {
                curr_ix = -1;
                fres = f_findfirst(&dir, &fileinfo, "", "*");
            }
        } else {
            fres = f_findnext(&dir, &fileinfo);
        }

        do {
            if (fres == FR_OK) {
                if (fileinfo.fname[0]) {
                    curr_ix++;
                    if (curr_ix < ix || ix < 0) {
                        fres = f_findnext(&dir, &fileinfo);
                    }
                } else {
                    fres = FR_NO_FILE;
                }
            }
        } while (fres == FR_OK && (curr_ix < ix || ix < 0));

        return curr_ix;
    }

    // count entries on folder (files and dirs)
    long count() {
        if (curr_folder_count < 0) {
            curr_folder_count = gotoIndex(-1) + 1;
        }
        return curr_folder_count;
    }

    // get entry index by filename
    int entryIdx(const char *name) {
        int cnt = 0;
        bool found = false;

        FRESULT fres = f_rewinddir(&dir);
        curr_ix = -1;
        if (fres == FR_OK) {
            fres = f_findfirst(&dir, &fileinfo, "", "*");
            do {
                if (fres == FR_OK) {
                    if (fileinfo.fname[0]) {
                        if (!strcicmp(fileinfo.lfname, name) || !strcicmp(fileinfo.fname, name)) {
                            found = true;
                        } else {
                            cnt++;
                            fres = f_findnext(&dir, &fileinfo);
                        }
                    } else {
                        fres = FR_NO_FILE;
                    }
                }
            } while (fres == FR_OK && !found);
        }
        return found ? cnt : 0; //stay at menu start if not found
    }

    // Get folder content entry by index
    bool entry(long idx, char *buf) {
        long l = gotoIndex(idx);

        if (l == idx) {
            if (fileinfo.fattrib & AM_DIR) {
                sprintf(buf, "%s/", fileinfo.lfname[0] ? fileinfo.lfname : fileinfo.fname);
            } else {
                sprintf(buf, "%s", fileinfo.lfname[0] ? fileinfo.lfname : fileinfo.fname);
            }
        }
        return l;
    }
};

class SDMenuT : public Menu::menuNode, public FSO {
public:
    char folderName[PATH_SIZE] = "/"; //set this to other folder when needed
    char selectedFolder[PATH_SIZE] = "/";
    char selectedFile[PATH_SIZE] = "";
    char focusedFolder[PATH_SIZE] = "/";
    char focusedFile[PATH_SIZE] = "";
    int8_t focusedFileIx = -1;
    bool canSelect = true;
    bool canDelete = true;

    void enable_selection() {
        this->canSelect = true;
    }

    void disable_selection() {
        this->canSelect = false;
    }

    void enable_deletion() {
        this->canDelete = true;
    }

    void disable_deletion() {
        this->canDelete = false;
    }

    // Using menuNode::menuNode
    // do not use default constructors as we wont allocate for data
    SDMenuT(constText *title, const char *at, Menu::action act = Menu::doNothing,
            Menu::eventMask mask = Menu::noEvent)
            : menuNode(title, 0, NULL, act, mask,
                       Menu::noStyle, (Menu::systemStyles) (Menu::_menuData | Menu::_canNav)) {
    }

    FRESULT begin() {
        return this->begin(folderName);
    }

    void refresh() {
        curr_folder_count = -1;
        count();
        //if (curr_folder_count>=focusedFileIx) {
        //    focusedFileIx = curr_folder_count-1;
        //}
    }

    FRESULT begin(const char *path) {
        if (lock_sd_card()) {
            extract_file_and_path(path, selectedFolder, selectedFile, PATH_SIZE);
            strncpy(folderName, selectedFolder, PATH_SIZE);
            FRESULT fres = FSO::openFolder(selectedFolder);
            if (fres == FR_OK) {
                // Select the file
                entryIdx(selectedFile);
            }
            return fres;
        } else {
            status::handleError(status::ST_ERROR, "SD card is locked");
            return FR_LOCKED;
        }
    }

    static void end() {
        unlock_sd_card();
    }

    //this requires latest menu version to virtualize data tables
    Menu::prompt &operator[](
            Menu::idx_t i) const override { return *(Menu::prompt *) this; }//this will serve both as menu and as its own prompt

    Menu::result sysHandler(SYS_FUNC_PARAMS) override {
        switch (event) {
            case Menu::enterEvent:
                if (nav.root->navFocus != nav.target) {// On sd card entry
                    // restore context
                    char *selectedFile = ((SDMenuT *) (&item))->selectedFile;
                    if (selectedFile[0]) {
                        nav.sel = ((SDMenuT *) (&item))->entryIdx(selectedFile) + 1;
                    } else {
                        nav.sel = 0;
                    }
                }
        }
        return Menu::proceed;
    }

    void focus(uint8_t i) {
        char fn[PATH_SIZE];
        SDMenuT::entry(i, fn);
        strcpy(focusedFile, fn);
        strcpy(focusedFolder, folderName);
        focusedFileIx = i;
        nav.node().event(Menu::refreshEvent);
    }

    void doNav(Menu::navNode &nav, Menu::navCmd cmd) override {

        char fn[PATH_SIZE];
        Menu::navCmd bubble_cmd = cmd;
        bool navigate = false;

        if (cmd.cmd == Menu::enterCmd && nav.sel == 0) { // [..] has been clicked -> Previous folder
            cmd.cmd = Menu::escCmd;
        }

        switch (cmd.cmd) {
            case Menu::idxCmd:  // Options

                // nav.event(enterEvent);

                // Show edit submenu
                if (nav.sel >= 1 && this->canDelete) {

                    SDMenuT::entry(nav.sel - 1, fn);

                    if (!endsWith(fn, "/")) {
                        nav.root->active().dirty = true;
                        nav.root->level++;
                        fileSubmenu.shadow->text = focusedFile;
                        nav.root->navFocus = nav.root->node().target = &fileSubmenu;
                        nav.root->node().sel = 0;
                    }
                }
                break;
            case Menu::enterCmd:
                if (nav.sel >= 1) {

                    SDMenuT::entry(nav.sel - 1, fn);

                    if (endsWith(fn, "/")) {
                        // Open folder (reusing the menu)
                        strcat(folderName, fn);
                        SDMenuT::openFolder(folderName);
                        dirty = true; // Redraw menu
                        nav.sel = 0;
                    } else {
                        if (this->canSelect) {
                            // Select a file and return
                            strcpy(selectedFile, fn);
                            strcpy(selectedFolder, folderName);
                            nav.root->node().event(Menu::updateEvent);
                            navigate = true;
                            bubble_cmd = Menu::escCmd;
                        }
                    }
                } else {
                    // [..]
                }
                break;
            case Menu::escCmd:
                if (!strlen(folderName) || !strcmp(folderName, "/")) {
                    // Root
                    nav.root->node().event(Menu::enterEvent);
                    bubble_cmd = Menu::escCmd;
                    navigate = true;
                } else {
                    // Previous folder
                    Menu::idx_t len = strlen(folderName);
                    if (len) {
                        folderName[len - 1] = '\0'; // remove last '/'
                        Menu::idx_t at =
                                (strrchr(folderName, '/') - folderName) + 1;   // search next index after the last '/'
                        memcpy(&fn, folderName + at, len - at); // copy last folder name
                        folderName[at] = '\0';
                    }
                    SDMenuT::openFolder(folderName);
                    dirty = true;//redraw menu
                    nav.sel = SDMenuT::entryIdx(fn) + 1;
                }

                break;
            case Menu::downCmd:
            case Menu::upCmd:
                navigate = true;
                break;
        }

        if (navigate) {
            menuNode::doNav(nav, bubble_cmd);
        }
    }

    // Print menu and items as this is a virtual data menu
    Menu::Used printTo(Menu::navRoot &root, bool sel, Menu::menuOut &out, Menu::idx_t idx, Menu::idx_t len,
                       Menu::idx_t pn) override {

        char fname[PATH_SIZE];

        if (root.navFocus != this) {


            // Show given title or filename if selected
            if (!strcmp(selectedFile, "")) {
                /*menuNode::printTo(root,sel,out,idx,len,pn)*/
                return out.printRaw(shadow->text, len);
            } else {
                len -= out.printRaw(shadow->text, len);
                len -= out.printRaw(": ", len);
                out.setColor(Menu::valColor, sel, Menu::enabledStatus, false);
                strncpy(fname, selectedFile, len);
                fname[len] = 0;
                return out.printRaw(fname, len);
            }
        } else if (idx == -1) {
            // When menu open (show folder name)

            ((Menu::menuNodeShadow *) shadow)->sz = SDMenuT::count() + 1;
            char *fn = folderName;
            return out.printRaw(fn, len);
        } else {

            Menu::idx_t i = out.tops[root.level] + idx;


            if (i < 1) {
                out.setColor(Menu::valColor, sel, Menu::enabledStatus, false);
                strcpy(fname, "[..]");
            } else {
                entry(i - 1, fname);
                if (endsWith(fname, "/")) {
                    out.setColor(Menu::valColor, sel, Menu::enabledStatus, false);
                }
            }

            // Changed focus
            if (sel) {
                if (i > 0 && (i - 1 != focusedFileIx || strcmp(fname, focusedFile) != 0)) {
                    focus(i - 1);
                }
            }

            fname[len] = 0; // just in case
            len -= out.printRaw(fname, len);
            return len;
        }
    }
};

extern SDMenuT filePicker;

#endif //TRX_FRONTEND_SD_FILEPICKER_MENU_H