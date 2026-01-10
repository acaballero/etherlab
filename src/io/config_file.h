//
// Created by Angel Dust on 05/09/2025.
//

#ifndef TRX_CONFIG_FILE_H
#define TRX_CONFIG_FILE_H

#include "config.h"
#include "fatfs/fatfs.h"
#include <cstdio>
#include <cstring>

#include <sys/types.h>

#define WRITE_FIELD(fmt, ...)                                                                                                                                  \
    snprintf(buf, sizeof(buf), fmt "\n", __VA_ARGS__);                                                                                                         \
    f_write(file, buf, strlen(buf), &bw)

template <typename T = st_config> class ConfigFile {
  public:
    bool save(const char *filename, const T *cfg);
    bool load(const char *filename, T *cfg, bool create_if_not_exists = false);

  protected:
    FIL *file = &FatFSFileHandle;

    char buf[256];
    bool read_int(const char *buf, int32_t *v);
    bool read_uint(const char *buf, uint32_t *v);
    bool read_int8(const char *buf, int8_t *v);
    bool read_uint8(const char *buf, uint8_t *v);
    bool read_int16(const char *buf, int16_t *v);
    bool read_uint16(const char *buf, uint16_t *v);
    bool read_int64(const char *buf, int64_t *v);
    bool read_uint64(const char *buf, uint64_t *v);
    bool read_float(const char *buf, float *v);
    bool read_bool(const char *buf, bool *v);
    bool read_string(const char *buf, char *v);

    void write_bin(const char *fmt, const uint8_t *data, size_t length);
    bool read_bin(const char *fmt, uint8_t *data, size_t length);

    bool read_line(const char *fmt);

    virtual bool save(const T *cfg);
    virtual bool load(T *cfg);
};

template <typename T> bool ConfigFile<T>::save(const char *filename, const T *cfg) {

    if (!lock_sd_card()) {
        return false;
    }

    if (f_open(file, filename, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
        return false;
    }

    bool b = save(cfg);

    f_close(file);

    unlock_sd_card();

    return b;
}

template <typename T> bool ConfigFile<T>::load(const char *filename, T *cfg, bool create_if_not_exists) {
    if (!lock_sd_card()) {
        return false;
    }

    auto mode = FA_READ | (create_if_not_exists ? FA_OPEN_ALWAYS : 0);

    bool b = f_open(file, filename, mode) == FR_OK;

    b = b && load(cfg);
    b = b && f_close(file) == FR_OK;

    unlock_sd_card();

    return b;
}

template <typename T> bool ConfigFile<T>::read_line(const char *fmt) {
    //  printf_("read_line: %s ", fmt);
    f_gets(buf, sizeof(buf), file);
    // printf_(">> %s\n", buf);
    return (std::strncmp(buf, fmt, strlen(fmt)) == 0);
}

template <typename T> bool ConfigFile<T>::read_int64(const char *fmt, int64_t *v) {
    if (read_line(fmt)) {
        *v = strtoll(buf + strlen(fmt), nullptr, 10);
        return true;
    } else {
        return false;
    }
}

template <typename T> bool ConfigFile<T>::read_uint64(const char *fmt, uint64_t *v) {
    if (read_line(fmt)) {
        *v = strtoull(buf + strlen(fmt), nullptr, 10);
        return true;
    } else {
        return false;
    }
}

template <typename T> bool ConfigFile<T>::read_int16(const char *fmt, int16_t *v) {
    if (read_line(fmt)) {
        *v = (int16_t)atoi(buf + strlen(fmt));
        return true;
    } else {
        return false;
    }
}

template <typename T> bool ConfigFile<T>::read_uint16(const char *fmt, uint16_t *v) {
    if (read_line(fmt)) {
        *v = (uint16_t)atoi(buf + strlen(fmt));
        return true;
    } else {
        return false;
    }
}

template <typename T> bool ConfigFile<T>::read_int8(const char *fmt, int8_t *v) {
    if (read_line(fmt)) {
        *v = (int8_t)atoi(buf + strlen(fmt));
        return true;
    } else {
        return false;
    }
}

template <typename T> bool ConfigFile<T>::read_uint8(const char *fmt, uint8_t *v) {
    if (read_line(fmt)) {
        *v = (uint8_t)atoi(buf + strlen(fmt));
        return true;
    } else {
        return false;
    }
}

template <typename T> bool ConfigFile<T>::read_int(const char *fmt, int32_t *v) {
    int64_t tmp;
    if (read_int64(fmt, &tmp)) {
        *v = (int32_t)tmp;
        return true;
    }
    return false;
}

template <typename T> bool ConfigFile<T>::read_uint(const char *fmt, uint32_t *v) {
    uint64_t tmp;
    if (read_uint64(fmt, &tmp)) {
        *v = (uint32_t)tmp;
        return true;
    }
    return false;
}

template <typename T> bool ConfigFile<T>::read_float(const char *fmt, float *v) {
    if (read_line(fmt)) {
        *v = strtof(buf + strlen(fmt), nullptr);
        return true;
    } else {
        return false;
    }
}

template <typename T> bool ConfigFile<T>::read_bool(const char *fmt, bool *v) {
    if (read_line(fmt)) {
        *v = strtof(buf + strlen(fmt), nullptr);
        return true;
    } else {
        return false;
    }
}

template <typename T> bool ConfigFile<T>::read_string(const char *fmt, char *v) {
    // printf_("read_string: %s\n", fmt);
    if (read_line(fmt)) {
        //  printf_("line: %s\n", buf);
        std::strcpy(v, buf + strlen(fmt));
        v[strlen(v) - 1] = 0; // remove newline
        return true;
    } else {
        //   printf_("Not found\n");
        return false;
    }
}

template <typename T> void ConfigFile<T>::write_bin(const char *fmt, const uint8_t *data, size_t length) {

    snprintf(buf, sizeof(buf), fmt);
    sprintf(buf + strlen(buf), "\n");

    UINT bw;
    f_write(file, buf, strlen(buf), &bw);

    for (size_t i = 0; i < length; ++i) {
        snprintf(buf, 3, "%02X", data[i]); // 2 digits per byte, 3 for null terminator
        f_write(file, buf, 2, &bw);
    }

    f_write(file, "\n", strlen("\n"), &bw);
}

template <typename T> bool ConfigFile<T>::read_bin(const char *fmt, uint8_t *data, size_t length) {

    if (!read_line(fmt)) {

        return false;
    }

    UINT br;
    FRESULT res;

    for (size_t i = 0; i < length; ++i) {
        res = f_read(file, buf, 2, &br);
        if (res == FR_OK && buf[0] != '\n' and buf[1] != '\n') {
            char byte_str[3] = {buf[0], buf[1], '\0'};
            data[i] = static_cast<uint8_t>(strtol(byte_str, nullptr, 16)); // Convert hex pair to byte
        } else {
            return false;
        }
    }

    res = f_read(file, buf, 1, &br);
    if (res == FR_OK && buf[0] == '\n') {
        return true;
    } else {
        return false;
    }
}

#endif // TRX_CONFIG_FILE_H
