//
// Created by Angel Dust on 15/12/2022.
//

#ifndef FSO_H
#define FSO_H

#include <cstdint>
#include <cstring>
#include <string>
#include "fatfs/fatfs.h"
#include "fatfs_file.h"
#include "ff.h"
#include "utils.hpp"
#include "status.h"

#define FILES_PER_PAGE 10

typedef struct {
    char name[FN_SIZE];
    DWORD size;
    BYTE attr;
    WORD fdate; // File date (FAT format)
    WORD ftime; // File time (FAT format)
} st_file_page_entry;

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
    bool opened{false}; // Could use other means to track this but having its own flag is safer

    FSO(const io::path &path = "") {

        file = &FatFSFileHandle;
        //  Assign the long filename memory
        fileinfo.lfname = lfname;
        fileinfo.lfsize = FN_SIZE;

        if (!path.empty()) {
            openFolder(path);
        }
    }

    virtual ~FSO() {
        close();
    }

    void close() {
        if (opened) {
            //  LOG("closing FSO\n");
            f_closedir(&dir);
            unlock_sd_card();
            opened = false;
        }
    }

    st_file_page_entry file_page[FILES_PER_PAGE];
    int page_top_ix = -1;

    /**
     * Gets the last updated file in the current folder
     */
    io::path get_last_updated_file() {
        FRESULT fres = f_rewinddir(&dir);
        if (fres != FR_OK) {
            return {};
        }

        FILINFO finfo;
        TCHAR lfn_buf[FN_SIZE];
        finfo.lfname = lfn_buf;
        finfo.lfsize = FN_SIZE;

        WORD newest_date = 0, newest_time = 0;
        int newest_index = -1, current_index = -1;
        char newest_name[FN_SIZE] = {0};

        while (f_readdir(&dir, &finfo) == FR_OK && finfo.fname[0]) {
            current_index++;

            // Skip directories
            if (finfo.fattrib & AM_DIR) {
                continue;
            }

            // Check if newer
            if (finfo.fdate > newest_date || (finfo.fdate == newest_date && finfo.ftime > newest_time)) {
                newest_date = finfo.fdate;
                newest_time = finfo.ftime;
                newest_index = current_index;

                const char *name = (finfo.lfname && finfo.lfname[0]) ? finfo.lfname : finfo.fname;
                strncpy(newest_name, name, FN_SIZE - 1);
                newest_name[FN_SIZE - 1] = 0;
            }
        }

        if (newest_index >= 0) {
            return io::path{newest_name};
        } else {
            return "";
        }
    }

    int read_page(DIR *dir, int start_ix, st_file_page_entry *buffer, int max_files) {
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
            buffer[found].fdate = finfo.fdate;
            buffer[found].ftime = finfo.ftime;
            found++;
        }

        return found; // Number of files loaded
    }

    /**
     * Opens the parent folder of a path
     */
    FRESULT openFolder(const io::path &path) {
        //   LOG("FSO: open folder %s\n", path.c_str());
        if (opened || lock_sd_card(5000)) {

            io::path folder = path.parent_path();

            FRESULT fres = f_opendir(&dir, folder.c_str());
            //    LOG("Opening folder '%s': Result: %d, index: %d, size: %d\n", folder.c_str(), fres, dir.index, folder.native().size());

            // reset everything so it is cached again
            curr_folder_count = -1;
            curr_ix = -1;
            page_top_ix = -1;

            opened = true;

            return fres;
        } else {
            status::handleError(status::ST_ERROR, "Error adquiring file system lock");
            return FR_LOCKED;
        }
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
            //  LOG("gotoIndex %d: Reading dir entry: %s,%d\n", ix, finfo.fname, finfo.lfsize);
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

        //  LOG("Getting entry index for file '%s' in current dir %d\n", name, dir.id);
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
        if (page_top_ix >= 0 && page_top_ix <= idx && idx <= min2(page_top_ix + FILES_PER_PAGE - 1, count() - 1)) {
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

#endif // FSO
