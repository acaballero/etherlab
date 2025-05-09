//
// Created by Angel Dust on 05/09/2025.
//

#ifndef TRX_CONFIG_FILE_H
#define TRX_CONFIG_FILE_H

#include "config.h"
#include <cstdio>
#include <cstring>

class ConfigFile {
  public:
    bool save(const char *filename, const st_config &cfg);
    bool load(const char *filename, st_config &cfg);
};

#endif // TRX_CONFIG_FILE_H
