//
// Created by Angel Dust on 15/12/2022.
//

#include "file_factory.h"
#include "wav.h"

std::unique_ptr<File> FileFactory::getFile(FileType ftype, char* path) {
    switch (ftype) {
        case FTYPE_WAV:
            return std::unique_ptr<File>(new WaveFile(path));

        case FTYPE_CS16:
        default:
            return std::unique_ptr<File>(new File(path));
    }
}

std::unique_ptr<File> FileFactory::getFile(char *path) {

    std::unique_ptr<File> file;

    FileType ftype = get_file_type_from_extension(path);

    if (ftype!=FTYPE_UNKNOWN) {
        file = getFile(ftype, path);
    }

    return file;
}
