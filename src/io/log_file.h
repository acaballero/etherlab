//
// Created by Angel Dust on 15/06/2025
//

#ifndef __LOG_FILE_H__
#define __LOG_FILE_H__

#include <string>
#include "fatfs_file.h"
#include "hw/stm32f4xx/rtc.h"

class LogFile {
  public:
    io::filesystem_error append(const io::path &filename) {
        auto result = ensure_directory(filename.parent_path());
        if (result.code()) {
            return {result};
        }

        return file.append(filename);
    }

    io::filesystem_error log(const std::string &entry);
    io::filesystem_error log(const st_datetime &datetime, const std::string &entry);

  private:
    FatFSFile file{};

    io::filesystem_error write_line(const std::string &message);
};

#endif /*__LOG_FILE_H__*/
