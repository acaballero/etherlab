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
    bool load(const char *filename, T *cfg);

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

#endif // TRX_CONFIG_FILE_H
