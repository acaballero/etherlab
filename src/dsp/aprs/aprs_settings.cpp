//
// Created by Angel Dust on 08/01/2026.
//
#include "io/config/config_file.h"
#include "dsp/aprs/aprs_settings.h"

template class ConfigFile<aprs::settings>;

template <> bool ConfigFile<aprs::settings>::save(const aprs::settings *cfg) {

    UINT bw;

    WRITE_FIELD("beacon_period_ms=%u", cfg->beacon_period_ms);
    WRITE_FIELD("deviation=%u", cfg->deviation);
    WRITE_FIELD("path=%s", cfg->path);
    WRITE_FIELD("message=%s", cfg->message);

    return true;
}

template <> bool ConfigFile<aprs::settings>::load(aprs::settings *cfg) {

    read_uint16("beacon_period_ms=", &cfg->beacon_period_ms);
    read_uint16("deviation=", &cfg->deviation);
    read_string("path=", cfg->path);
    read_string("message=", cfg->message);

    return true;
}
