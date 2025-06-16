//
// Created by Angel Dust on 15/06/2025
//

#include "log_file.h"
#include "hw/stm32f4xx/rtc.h"
#include "io/fatfs_file.h"

io::filesystem_error LogFile::log(const std::string &entry) {
    return log(rtc_get_date_time(), entry);
}

io::filesystem_error LogFile::log(const st_datetime &datetime, const std::string &entry) {
    char buff[12];
    rtc_to_string(datetime, false, buff);
    return write_line(std::string(buff) + " : " + entry);
}

io::filesystem_error LogFile::write_line(const std::string &message) {
    auto error = file.write_line(message);
    if (error.ok()) {
        file.sync();
    }
    return error;
}
