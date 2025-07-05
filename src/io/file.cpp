//
// Created by Angel Dust on 14/12/2022.
//

#include "file.h"
#include "file_types.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "fatfs/fatfs.h"
#include "status.h"

File::File(io::path &filepath) {
    path = {filepath};
}

FRESULT File::open(WaveInfo &wi) {

    FRESULT fres = FR_OK;
    wi = get_info_from_file_path(path);
    wi.n_channels = 2;
    wi.bits_sample = 16;

    if (get_file_type_from_extension(path) == FTYPE_CS16) {

        mode = FA_READ;

        fres = f_open(fil, path.c_str(), mode);

        wi.format = FSTATUS_OK;

        return fres;
    }

    wi.format = FSTATUS_INVALID;
    return fres;
}

FRESULT File::create(WaveInfo wi) {

    mode = FA_WRITE | FA_OPEN_ALWAYS | FA_CREATE_ALWAYS;

    FRESULT fres = f_open(fil, path.c_str(), mode);

    return fres;
}

FRESULT File::read(char *p, uint32_t count) {

    UINT bytes_read;

    FRESULT fres = f_read(fil, p, count, &bytes_read);

    return fres;
}

FRESULT File::close() {
    FRESULT fres;
    fres = f_close(fil);
    mode = 0;
    return fres;
}

FRESULT File::write(char *p, uint32_t count) {
    FRESULT fres;
    UINT bytes_written;

    fres = f_write(fil, p, count, &bytes_written);

    return fres;
}

bool File::is_open() {
    return mode != 0;
}

File::~File() {
    close();
}
