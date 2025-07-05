//
// Created by Angel Dust on 14/12/2022.
//

#ifndef TRX_FRONTEND_FILE_H
#define TRX_FRONTEND_FILE_H

#include "io/fatfs_file.h"
#include "stdio.h"
#include "file_types.h"
#include "fatfs/fatfs.h"
#include "../../lib/FatFs/ff.h"

class File {

  public:
    File(io::path &);

    virtual ~File();

    virtual FRESULT open(WaveInfo &wi);

    virtual FRESULT close();

    virtual FRESULT create(WaveInfo wi);

    virtual FRESULT write(char *p, uint32_t count);

    virtual FRESULT read(char *p, uint32_t count);

    io::path get_path() {
        return path;
    }

    virtual bool is_open();

  protected:
    FIL *fil = &FatFSFileHandle;
    io::path path;
    uint8_t mode = 0;
};

#endif // TRX_FRONTEND_FILE_H
