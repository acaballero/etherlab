//
// Created by Angel Dust on 14/12/2022.
//

#ifndef TRX_FRONTEND_FILE_TYPES_H
#define TRX_FRONTEND_FILE_TYPES_H

#include "fatfs_file.h"
#include "stdio.h"

enum FileType { FTYPE_WAV = 0, FTYPE_CS16, FTYPE_LAST, FTYPE_UNKNOWN };

extern const char *file_type_names[];
extern const char *file_type_extensions[];

enum FileStatus { FSTATUS_NONE = 0, FSTATUS_INVALID, FSTATUS_ERROR, FSTATUS_OK };

struct WaveInfo {

    FileStatus format;
    uint64_t carrier_freq{0};
    uint16_t n_channels{2};
    uint32_t sample_rate{0};
    uint16_t bits_sample{16};
    uint32_t byte_rate{0}; // = n_channels * sampling_rate * bits_sample / 8
    uint32_t file_size{0};
};

#define WAVEFILE_DEFAULT_FOLDER "captures"
#define WAVEFILE_DEFAULT_FILENAME "cap%d_%dsps_%dkhz.%s"

FileType get_file_type_from_extension(const io::path &);
WaveInfo get_info_from_file_path(const io::path &);

#endif // TRX_FRONTEND_FILE_TYPES_H
