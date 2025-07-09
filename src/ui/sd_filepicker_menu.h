#ifndef TRX_FRONTEND_SD_FILEPICKER_MENU_H
#define TRX_FRONTEND_SD_FILEPICKER_MENU_H

#include <cstring>
#include <status.h>
#include <string>
#include <sys/_stdint.h>
#include "Display_afb.h"
#include "hw/stm32.h"
#include "menuBase.h"
#include "ui/menu.h"
#include "../../lib/Menu/src/menu.h"
#include "../../lib/FatFs/ff.h"
#include "../fatfs/fatfs.h"
#include "../../lib/utils/utils.hpp"
#include "io/file_types.h"
#include "menu_actions.h"
#include "ui/ui_types.h"
#include "ui/view_manager.h"
#include "ui/modal_view.h"

#define FILES_PER_PAGE 10

typedef struct {
    char name[FN_SIZE];
    DWORD size;
    BYTE attr;
} st_file_page_entry;

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

    st_file_page_entry file_page[FILES_PER_PAGE];
    int page_top_ix = -1;

    int readDirectoryPage(DIR *dir, int start_ix, st_file_page_entry *buffer, int max_files) {
        FRESULT fres;
        FILINFO finfo;
        TCHAR lfn_buf[FN_SIZE]; // 128 bytes
        finfo.lfname = lfn_buf;
        finfo.lfsize = sizeof(lfn_buf);

        int curr_ix = -1;
        int found = 0;

        fres = f_rewinddir(dir); // Always start from the top
        if (fres != FR_OK) {
            return -1;
        }

        while (found < max_files) {
            fres = f_readdir(dir, &finfo);
            if (fres != FR_OK || !finfo.fname[0]) {
                break; // End of dir
            }

            curr_ix++;
            if (curr_ix < start_ix) {
                continue; // Skip until desired index
            }

            // Use LFN if available, otherwise fallback to SFN
            const char *src_name = (finfo.lfname && finfo.lfname[0]) ? finfo.lfname : finfo.fname;

            strncpy(buffer[found].name, src_name, FN_SIZE - 1);
            buffer[found].name[FN_SIZE - 1] = 0;

            buffer[found].size = finfo.fsize;
            buffer[found].attr = finfo.fattrib;

            found++;
        }

        return found; // Number of files loaded
    }

    /**
     * Opens the parent folder of a path
     */
    FRESULT openFolder(io::path path) {

        io::path folder = path.parent_path();

        FRESULT fres = f_opendir(&dir, folder.c_str());
        //   LOG("Opening folder '%s': Result: %d, index: %d, size: %d\n", folder.c_str(), fres, dir.index, folder.native().size());

        // reset everything so it is cached again
        curr_folder_count = -1;
        curr_ix = -1;
        page_top_ix = -1;
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
    int count() {
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
    bool entry(int idx, char *buf = nullptr, int size = 0) {
        if (idx < 0) {
            return false;
        }

        int l;
        if (page_top_ix <= idx && idx <= min2(page_top_ix + FILES_PER_PAGE - 1, count())) {
            st_file_page_entry entry = file_page[idx - page_top_ix];
            fileinfo.lfname = entry.name;
            fileinfo.fattrib = entry.attr;
            fileinfo.fsize = entry.size;
            l = idx;
        } else {
            l = gotoIndex(idx);
        }

        if (l == idx && buf) {
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
    std::vector<int> sel_items;

    Menu::menu_action_st menu_actions_arr[6];
    Menu::menu_actions_st menu_actions = {menu_actions_arr, 6};

    enum menu_actions_type { UP, DOWN, ENTER, BACK, OPEN, DELETE };
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

    bool is_dir(BYTE attr) {
        return attr & AM_DIR;
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

    void delete_file() {

        std::string message_str;

        if (sel_items.empty() && nav.node().sel >= 1 && this->can_delete) {
            char fn[FN_SIZE];
            SDMenuT::entry(nav.node().sel - 1, fn, sizeof(fn));
            if (!(is_dir(fileinfo.fattrib))) {
                toggle_select(nav.node().sel - 1);
            }
        }

        if (!sel_items.empty()) {
            message_str = {"Delete " + std::to_string(sel_items.size()) + " file/s?"};
            view_manager::open(std::make_unique<ModalView>("Delete", message_str, modal_t::YESNO, [this](bool ok) {
                if (ok) {

                    for (const auto ix : sel_items) {
                        char fn[FN_SIZE];
                        entry(ix, fn, sizeof(fn));
                        io::path file_path = selected_path.parent_path() / fn;

                        LOG("Deleting file '%s'\n", file_path.c_str());
                        FRESULT res = f_unlink(file_path.c_str());

                        if (res != FR_OK) {
                            status::handleError(status::ST_ERROR, "Error deleting file");
                        }
                    }

                    clear_selection();
                    refresh();
                }
            }));
        }
    }

    void open_file() {
        if (nav.node().sel >= 1 && this->can_select) {
            // Select current file and return
            nav.node().event(Menu::updateEvent);
            nav.doNav(Menu::upCmd);
        }
    }

    void clear_selection() {
        sel_items.clear();
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
                readDirectoryPage(&dir, 0, file_page, FILES_PER_PAGE);
                page_top_ix = 0;
            }

            int i = 0;
            for (i = 0; i < Menu::navigation_actions.size; i++) {
                menu_actions_arr[i] = Menu::navigation_actions_arr[i];
            }

            menu_actions_arr[OPEN] = {"Open", [this]() {
                                          open_file();
                                      }};
            menu_actions_arr[DELETE] = {"Delete", [this]() {
                                            delete_file();
                                        }};

            clear_selection();
            update();

            return fres;
        } else {
            status::handleError(status::ST_ERROR, "SD card is locked");
            return FR_LOCKED;
        }
    }

    static void end() {
        actions_signal.emit(nullptr);
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
        // LOG("focus:%s,%d\n", fn, i);
        focused_file_ix = i;
        nav.node().event(Menu::refreshEvent);
    }

    void toggle_select(int item) {

        auto it = std::find(sel_items.begin(), sel_items.end(), item);
        if (it != sel_items.end()) {
            // Item exists, remove it
            sel_items.erase(it);
        } else {
            // Item doesn't exist, add it
            sel_items.push_back(item);
        }
    }

    bool is_selected(int item) {
        return std::find(sel_items.begin(), sel_items.end(), item) != sel_items.end();
    }

    void update() {

        LOG("%d\n", HAL_GetTick());
        auto sel = nav.node().sel;
        entry(sel - 1);

        menu_actions_arr[OPEN].enabled = sel && sel_items.empty() && can_select;
        menu_actions_arr[DELETE].enabled = (!sel_items.empty() || can_delete) && !is_dir(fileinfo.fattrib) && sel;

        actions_signal.emit(&menu_actions);
        LOG("%d,d:%d\n", HAL_GetTick(), menu_actions_arr[DELETE].enabled);
    }

    void doNav(Menu::navNode &nav, Menu::navCmd cmd) override {

        io::path folder = selected_path.parent_path();
        char fn[FN_SIZE];
        Menu::navCmd bubble_cmd = cmd;
        bool navigate = false;

        if (cmd.cmd == Menu::enterCmd && nav.sel == 0) { // [..] has been clicked -> Previous folder
            cmd.cmd = Menu::escCmd;
        }

        switch (cmd.cmd) {
            case Menu::idxCmd: // Options
                // nav.event(enterEvent);
                // Show edit submenu
                delete_file();
                break;
            case Menu::enterCmd:
                if (nav.sel >= 1) {

                    SDMenuT::entry(nav.sel - 1, fn, sizeof(fn));
                    selected_path = folder / fn;

                    if (is_dir(fileinfo.fattrib)) {

                        // Open folder (reusing the menu)
                        //  LOG("enter: parent %s\n", fn);
                        selected_path /= "";

                        SDMenuT::openFolder(selected_path);
                        dirty = true; // Redraw menu
                        nav.sel = 0;
                        clear_selection();
                    } else {
                        if (this->can_select) {
                            // Select a file and return
                            toggle_select(nav.sel - 1);
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
                    clear_selection();
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

        update();
    }

    // Print menu and items as this is a virtual data menu
    Menu::Used printTo(Menu::navRoot &root, bool sel, Menu::menuOut &out, Menu::idx_t idx, Menu::idx_t len, Menu::idx_t pn) override {

        bool show_parent = out.tops[root.level] == 0;

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
            int top = out.tops[root.level];
            top = top < 2 ? 0 : top - 1;

            Menu::idx_t i = top + idx;
            st_file_page_entry *fentry;
            char buff[FN_SIZE + 4];

            if (top != page_top_ix) { // update current page
                readDirectoryPage(&dir, top, file_page, FILES_PER_PAGE);
                page_top_ix = top;
            }

            int pg_index = i - page_top_ix - (show_parent ? 1 : 0);

            if (show_parent && i == 0) {
                out.setColor(Menu::valColor, sel, Menu::enabledStatus, false);
                return out.printRaw("[..]", len);
            } else {
                fentry = &file_page[pg_index];
            }

            i = i - (show_parent ? 1 : 0);
            // Changed focus
            if (sel) {
                LOG("sel,%d,%d\n", i, focused_file_ix);
                if ((i != focused_file_ix || strcmp(fentry->name, focused_path.filename().c_str()) != 0)) {
                    LOG("focus\n");
                    focus(i);
                }
            }

            bool is_dir = fentry->attr & AM_DIR;
            auto color = is_dir ? Menu::valColor : Menu::fgColor;

            if (is_selected(i)) {
                out.setColor(Menu::titleColor, sel);
            } else {

                out.setColor(color, sel);
            }

            snprintf(buff, sizeof(buff), "[%2d] ", i + 1);
            len -= out.printRaw(buff, len);

            out.setColor(color, sel);
            snprintf(buff, sizeof(buff), "%s", fentry->name);
            len -= out.printRaw(buff, len);

            if (is_dir) {
                len -= out.printRaw("/", len);
            }
        }

        return len;
    }
};

extern SDMenuT filePicker;

#endif // TRX_FRONTEND_SD_FILEPICKER_MENU_H
