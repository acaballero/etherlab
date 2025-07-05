#ifndef TRX_FRONTEND_SD_FILEPICKER_MENU_H
#define TRX_FRONTEND_SD_FILEPICKER_MENU_H

#include <cstring>
#include <status.h>
#include "hw/stm32.h"
#include "ui/menu.h"
#include "../../lib/Menu/src/menu.h"
#include "../../lib/FatFs/ff.h"
#include "../fatfs/fatfs.h"
#include "../../lib/utils/utils.hpp"
#include "io/file_types.h"
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
        //  Assign the long filename memory
        fileinfo.lfname = lfname;
        fileinfo.lfsize = FN_SIZE;
    }

    virtual ~FSO() {
        //   LOG("Closing FSO\n");
        f_closedir(&dir);
    }

    /**
     * Opens the parent folder of a path
     */
    FRESULT openFolder(io::path path) {

        io::path folder = path.parent_path();

        FRESULT fres = f_opendir(&dir, folder.c_str());
        //   LOG("Opening folder '%s': Result: %d, index: %d, size: %d\n", folder.c_str(), fres, dir.index, folder.native().size());

        curr_folder_count = -1; // reset folder count
        curr_ix = -1;
        return fres;
    }

    // Go to a file with index 'ix' in the current dir or to the last file if 'ix' is negative
    // Returns the stop index
    // The file info gets stored in the 'fileinfo' member
    long gotoIndex(int16_t ix) {
        FRESULT fres;
        FILINFO finfo;
        TCHAR local_lfname[FN_SIZE + 1]; // finfo needs a buffer
        finfo.lfname = local_lfname;
        finfo.lfsize = FN_SIZE;

        if (ix == curr_ix && ix > -1) {
            return curr_ix;
        }

        if (ix <= curr_ix || curr_ix == -1) { // cursor is past the target?
            fres = f_rewinddir(&dir);

            if (fres == FR_OK) {
                curr_ix = -1;
            }
        }

        do {
            fres = f_readdir(&dir, &finfo);
            //    LOG("gotoIndex %d: Reading dir entry: %s,%d\n", ix, finfo.fname, finfo.lfsize);
            if (fres == FR_OK) {
                if (finfo.fname[0]) {
                    curr_ix++;

                    // Copy current file info and restore long name buffer
                    fileinfo = finfo;
                    fileinfo.lfname = lfname;
                    if (finfo.lfname && finfo.lfname[0]) {

                        int len = strlen(finfo.lfname);
                        strncpy(fileinfo.lfname, finfo.lfname, len);
                        fileinfo.lfname[len] = 0;
                        //   LOG("%d,%d,fileinfo.lfname:%s\n", ix, curr_ix, fileinfo.lfname);
                    } else {
                        fileinfo.lfname[0] = 0;
                    }

                    if (curr_ix >= ix && ix >= 0) {
                        //        LOG("Found file at index %d\n", curr_ix);

                        break;
                    }
                } else {
                    // End of directory
                    break;
                }
            }
        } while (fres == FR_OK);

        //    LOG("gotoIndex %d: %d\n", ix, curr_ix);
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

        // LOG("Getting entry index for file '%s' in current dir %d\n", name, dir.index);
        FRESULT fres = f_rewinddir(&dir);
        curr_ix = -1;
        if (fres == FR_OK) {
            // Use f_readdir instead of f_findfirst/f_findnext
            do {

                fres = f_readdir(&dir, &fileinfo);
                //  LOG("entryIdx: Reading dir entry: %s,%d\n", fileinfo.fname, fileinfo.lfsize);
                if (fres == FR_OK) {
                    if (fileinfo.fname[0]) {
                        //   LOG("Comparing '%s' with '%s' and '%s'\n", name, fileinfo.lfname, fileinfo.fname);
                        if ((strlen(fileinfo.lfname) && !strcicmp(fileinfo.lfname, name)) || (strlen(fileinfo.fname) && !strcicmp(fileinfo.fname, name))) {

                            found = true;
                        } else {
                            cnt++;
                        }
                    } else {
                        // End of directory
                        break;
                    }
                }
            } while (fres == FR_OK && !found);
        }
        //  LOG("Result: found: %d in %d\n", found, cnt);
        return found ? cnt : 0; // stay at menu start if not found
    }

    // Get folder content entry by index
    bool entry(long idx, char *buf, size_t size) {

        long l = gotoIndex(idx);

        if (l == idx) {
            TCHAR *fname = fileinfo.lfname[0] ? fileinfo.lfname : fileinfo.fname;
            if (fileinfo.fattrib & AM_DIR) {
                snprintf(buf, size, "%s", fname);
            } else {
                snprintf(buf, size, "%s", fname);
            }
        }

        return l;
    }
};

class SDMenuT : public Menu::menuNode, public FSO {
  public:
    io::path selected_path = "/";
    io::path focused_path = "/";
    int8_t focused_file_ix = -1;
    bool can_select = true;
    bool can_delete = true;

    void enable_selection() {
        this->can_select = true;
    }

    void disable_selection() {
        this->can_select = false;
    }

    void enable_deletion() {
        this->can_delete = true;
    }

    void disable_deletion() {
        this->can_delete = false;
    }

    // Using menuNode::menuNode
    // do not use default constructors as we wont allocate for data
    SDMenuT(constText *title, const char *at, Menu::action act = Menu::doNothing, Menu::eventMask mask = Menu::noEvent)
        : menuNode(title, 0, NULL, act, mask, Menu::noStyle, (Menu::systemStyles)(Menu::_menuData | Menu::_canNav)) {
    }

    FRESULT begin() {
        return this->begin(selected_path);
    }

    void refresh() {
        curr_folder_count = -1;
        count();
        // if (curr_folder_count>=focusedFileIx) {
        //     focusedFileIx = curr_folder_count-1;
        // }
    }

    FRESULT begin(io::path &path) {
        if (lock_sd_card()) {

            FRESULT fres = FSO::openFolder(path);
            if (fres == FR_OK) {
                selected_path = path.parent_path() + "/";
                // Select the file
                if (entryIdx(path.filename().c_str())) {
                    selected_path /= path.filename();
                }
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

    // this requires latest menu version to virtualize data tables
    Menu::prompt &operator[](Menu::idx_t i) const override {
        return *(Menu::prompt *)this;
    } // this will serve both as menu and as its own prompt

    Menu::result sysHandler(SYS_FUNC_PARAMS) override {
        switch (event) {
            case Menu::enterEvent:
                if (nav.root->navFocus != nav.target) { // On sd card entry
                    // restore context
                    const char *filename = ((SDMenuT *)(&item))->selected_path.filename().c_str();
                    if (filename[0]) {
                        nav.sel = ((SDMenuT *)(&item))->entryIdx(filename) + 1;
                    } else {
                        nav.sel = 0;
                    }
                }
            default:
                break;
        }
        return Menu::proceed;
    }

    void focus(uint8_t i) {
        char fn[FN_SIZE];
        SDMenuT::entry(i, fn, sizeof(fn));
        focused_path = selected_path.parent_path() / fn;

        focused_file_ix = i;
        nav.node().event(Menu::refreshEvent);
    }

    void doNav(Menu::navNode &nav, Menu::navCmd cmd) override {

        io::path folder = selected_path.parent_path();
        char fn[FN_SIZE];
        Menu::navCmd bubble_cmd = cmd;
        bool navigate = false;
        int n;

        if (cmd.cmd == Menu::enterCmd && nav.sel == 0) { // [..] has been clicked -> Previous folder
            cmd.cmd = Menu::escCmd;
        }

        switch (cmd.cmd) {
            case Menu::idxCmd: // Options

                // nav.event(enterEvent);

                // Show edit submenu
                if (nav.sel >= 1 && this->can_delete) {

                    SDMenuT::entry(nav.sel - 1, fn, sizeof(fn));

                    if (fileinfo.fattrib & AM_DIR) {
                        nav.root->active().dirty = true;
                        nav.root->level++;
                        fileSubmenu.shadow->text = focused_path.filename().c_str();
                        nav.root->navFocus = nav.root->node().target = &fileSubmenu;
                        nav.root->node().sel = 0;
                    }
                }
                break;
            case Menu::enterCmd:
                if (nav.sel >= 1) {

                    SDMenuT::entry(nav.sel - 1, fn, sizeof(fn));
                    selected_path = folder / fn;

                    if (fileinfo.fattrib & AM_DIR) {

                        // Open folder (reusing the menu)
                        //  LOG("enter: parent %s\n", fn);
                        selected_path /= "";

                        SDMenuT::openFolder(selected_path);
                        dirty = true; // Redraw menu
                        nav.sel = 0;
                    } else {
                        if (this->can_select) {
                            // Select a file and return
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

                if (folder.empty() || folder.filename().empty()) {
                    // Root
                    nav.root->node().event(Menu::enterEvent);
                    bubble_cmd = Menu::escCmd;
                    navigate = true;
                } else {
                    // Previous folder

                    selected_path = selected_path.parent_path().parent_path(); // 1st up: the folder + one up, the parent folder
                    SDMenuT::openFolder(selected_path);
                    nav.sel = SDMenuT::entryIdx(folder.filename().c_str()) + 1;
                    dirty = true; // redraw menu
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
    Menu::Used printTo(Menu::navRoot &root, bool sel, Menu::menuOut &out, Menu::idx_t idx, Menu::idx_t len, Menu::idx_t pn) override {

        char fname[FN_SIZE];

        if (root.navFocus != this) {

            // Show given title or filename if selected
            if (selected_path.filename().empty() || !can_select) {
                /*menuNode::printTo(root,sel,out,idx,len,pn)*/
                return out.printRaw(shadow->text, len);
            } else { // Only shows the file if the selection is enabled
                len -= out.printRaw(shadow->text, len);
                len -= out.printRaw(": ", len);
                out.setColor(Menu::valColor, sel, Menu::enabledStatus, false);
                return out.printRaw(selected_path.filename().c_str(), len);
            }

        } else if (idx == -1) {
            // When menu open (show folder name)

            ((Menu::menuNodeShadow *)shadow)->sz = SDMenuT::count() + 1;

            auto str = selected_path.parent_path().c_str();
            return out.printRaw(str[0] ? str : "/", len);
        } else {

            Menu::idx_t i = out.tops[root.level] + idx;

            if (i < 1) {
                out.setColor(Menu::valColor, sel, Menu::enabledStatus, false);
                strcpy(fname, "[..]");
            } else {
                entry(i - 1, fname, sizeof(fname));
                if (fileinfo.fattrib & AM_DIR) {
                    out.setColor(Menu::valColor, sel, Menu::enabledStatus, false);
                }
            }

            // Changed focus
            if (sel) {
                if (i > 0 && (i - 1 != focused_file_ix || strcmp(fname, focused_path.filename().c_str()) != 0)) {
                    focus(i - 1);
                }
            }

            fname[len] = 0; // just in case
            len -= out.printRaw(fname, len);
            if (fileinfo.fattrib & AM_DIR) {
                len -= out.printRaw("/", len);
            }
        }

        return len;
    }
};

extern SDMenuT filePicker;

#endif // TRX_FRONTEND_SD_FILEPICKER_MENU_H
