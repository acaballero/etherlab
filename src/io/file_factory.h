//
// Created by Angel Dust on 15/12/2022.
//

#ifndef TRX_FRONTEND_FILE_FACTORY_H
#define TRX_FRONTEND_FILE_FACTORY_H

#include <memory>
#include "file_types.h"
#include "file.h"

class FileFactory {

  public:
    static std::unique_ptr<File> getFile(FileType ftype, io::path &path);
    static std::unique_ptr<File> getFile(io::path &path);
};

#endif // TRX_FRONTEND_FILE_FACTORY_H
