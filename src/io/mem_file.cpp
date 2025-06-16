//
// Created by Angel Dust on 05/09/2025.
//

#include "config_file.h"
#include "printf.h"
#include "types.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

template <> bool ConfigFile<st_freq_mem>::save(const st_freq_mem *cfg) {

    UINT bw;

    for (int i = 0; i < FREQ_MEM_SIZE; ++i) {
        WRITE_FIELD("freq_mem[%d].group=%u", i, cfg[i].group);
        WRITE_FIELD("freq_mem[%d].id=%d", i, cfg[i].id);
        WRITE_FIELD("freq_mem[%d].freq=%u", i, cfg[i].freq);
        WRITE_FIELD("freq_mem[%d].mode=%d", i, cfg[i].mode);
        WRITE_FIELD("freq_mem[%d].name=%s", i, cfg[i].name);
    }

    return true;
}

template <> bool ConfigFile<st_freq_mem>::load(st_freq_mem *cfg) {

    char fmt[50];

    // Read frequency memory array

    for (int i = 0; i < FREQ_MEM_SIZE; ++i) {
        sprintf(fmt, "freq_mem[%d].group=", i);
        read_uint16(fmt, &cfg[i].group);
        sprintf(fmt, "freq_mem[%d].id=", i);
        read_int(fmt, &cfg[i].id);
        sprintf(fmt, "freq_mem[%d].freq=", i);
        read_uint64(fmt, &cfg[i].freq);
        sprintf(fmt, "freq_mem[%d].mode=", i);
        read_int(fmt, (int32_t *)&cfg[i].mode);
        sprintf(fmt, "freq_mem[%d].name=", i);
        read_string(fmt, cfg[i].name);
    }

    return true;
}
