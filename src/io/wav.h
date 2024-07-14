//
// Created by Angel Dust on 15/06/2021.
//

#ifndef TRX_FRONTEND_WAV_H
#define TRX_FRONTEND_WAV_H

#include "stdio.h"
#include "file_types.h"
#include "file.h"
#include "string.h"
#include "fatfs/fatfs.h"
#include "../../lib/FatFs/ff.h"

struct fmt_pcm_t {
    constexpr fmt_pcm_t(){}
    constexpr fmt_pcm_t(const uint32_t sampling_rate,const uint16_t n_channels) :
            nChannels{n_channels},
            nSamplesPerSec{sampling_rate},
            nAvgBytesPerSec{sampling_rate * 2 * n_channels},
            nBlockAlign{static_cast<uint16_t>(n_channels*16/8)}
    {    }

public:
    uint8_t ckID[4]{'f', 'm', 't', ' '};
    uint32_t cksize{16};
    uint16_t wFormatTag{0x0001};
    uint16_t nChannels{2};
    uint32_t nSamplesPerSec{0};
    uint32_t nAvgBytesPerSec{0};
    uint16_t nBlockAlign{2};
    uint16_t wBitsPerSample{16};
}; // 24 bytes lenght (8+16)

struct data_t {
    constexpr data_t() {}
    constexpr data_t(const uint32_t size) : cksize{size} {}

public:
    uint8_t ckID[4]{'d', 'a', 't', 'a'};
    uint32_t cksize{0};
}; // 8 bytes

struct header_t {

    constexpr header_t(){};

    constexpr header_t(
            const uint32_t sampling_rate,
            const uint16_t n_channels,
            const uint32_t data_chunk_size,
            const uint32_t info_chunk_size
    ) : cksize{sizeof(header_t) + data_chunk_size + info_chunk_size - 8},
        fmt{sampling_rate,n_channels},
        data{data_chunk_size} {}

public:
    uint8_t riff_id[4]{'R', 'I', 'F', 'F'};
    uint32_t cksize{0};
    uint8_t wave_id[4]{'W', 'A', 'V', 'E'};
    fmt_pcm_t fmt;
    data_t data;
}; // 44 bytes (12+24+8)

struct tags_t {
    tags_t(){}
    tags_t(const char *str) {
        strcpy(&(title[0]), str);
        cksize = sizeof(tags_t) - 8;
    }

public:
    uint8_t list_id[4]{'L', 'I', 'S', 'T'};
    uint32_t cksize{0};
    uint8_t info_id[4]{'I', 'N', 'F', 'O'};
    uint8_t iart_id[4]{'I', 'A', 'R', 'T'};
    uint32_t sckiart_size{12};
    char artist[12]{"AngelDust\0\0"};
    uint8_t inam_id[4]{'I', 'N', 'A', 'M'};
    uint32_t sckinam_size{64};
    char title[64]{0};
}; // 104 bytes (40+64)

class WaveFile : public File {

public:

    WaveFile(char *filepath) : File(filepath) { };

    FRESULT open(WaveInfo &wi) override;
    FRESULT close() override;
    FRESULT create(WaveInfo wi) override;
    FRESULT write(char *p,uint32_t count) override;
    FRESULT read(char *p,uint32_t count) override;

protected:

    WaveInfo info;
    size_t tags_size;
    size_t data_size;
    header_t header {};
    uint32_t data_start;
    FRESULT update_header();
    FRESULT write_tags();

};

#endif //TRX_FRONTEND_WAV_H
