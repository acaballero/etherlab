//
// Created by Angel Dust on 15/06/2021.
//

#include "wav.h"
#include "stdio.h"
#include "stdlib.h"
#include "fatfs/fatfs.h"
#include "status.h"

FRESULT WaveFile::open(WaveInfo &wi) {

    mode = FA_READ;
    data_size = 0;
    FRESULT fres = f_open(fil, path.c_str(), mode);
    UINT bytes_read;
    wi.format = FSTATUS_ERROR;

    if (fres == FR_OK) {

        size_t i = 0;
        char ch;
        const uint8_t tag_INAM[4] = {'I', 'N', 'A', 'M'};
        char title_buffer[32];
        uint32_t riff_size, data_end, title_size;
        size_t search_limit = 0;

        f_read(fil, &header, sizeof(header), &bytes_read); // Read header (RIFF and WAVE)

        riff_size = header.cksize + 8;       // 8 bytes for the 2 first fields not included in cksize
        data_start = header.fmt.cksize + 28; // always 44 (16+28) for PCM format
        data_end = data_start + header.data.cksize;

        // INAM parse

        if (data_end < riff_size) {

            fres = f_lseek(fil, data_end);

            if (fres == FR_OK) {
                while (f_read(fil, &ch, 1, &bytes_read) == FR_OK) {

                    if (ch == tag_INAM[i++]) {
                        if (i == 4) {
                            // Tag found, copy title
                            fres = f_read(fil, &title_size, sizeof(uint32_t), &bytes_read);
                            if (fres == FR_OK) {
                                if (title_size > 32) {
                                    title_size = 32;
                                }
                                fres = f_read(fil, &title_buffer, title_size, &bytes_read);

                                if (fres == FR_OK) {
                                    char *endptr;
                                    info.carrier_freq = strtol(title_buffer, &endptr, 10);
                                }
                            }
                            break;
                        }
                    } else {
                        if (ch == tag_INAM[0]) {
                            i = 1;
                        } else {
                            i = 0;
                        }
                    }
                    if (search_limit == 256) {
                        break;
                    } else {
                        search_limit++;
                    }
                }
            }

            info.sample_rate = header.fmt.nSamplesPerSec;
            info.bits_sample = header.fmt.wBitsPerSample;
            info.byte_rate = header.fmt.nAvgBytesPerSec;
            info.file_size = fil->fsize;

            fres = f_lseek(fil, data_start);

            if (fres == FR_OK) {
                info.format = FSTATUS_OK;
            }

            wi = info;
        } else {
            wi.format = FSTATUS_INVALID;
            fres = FR_INVALID_OBJECT;
        }
    }

    return fres;
}

FRESULT WaveFile::create(WaveInfo wi) {

    mode = FA_WRITE | FA_OPEN_ALWAYS | FA_CREATE_ALWAYS;
    data_size = 0;

    FRESULT fres = f_open(fil, path.c_str(), mode);

    this->info = wi;

    if (fres == FR_OK) {
        fres = update_header();
    }

    return fres;
}

FRESULT WaveFile::read(char *p, uint32_t count) {

    UINT bytes_read;

    FRESULT fres = f_read(fil, p, count, &bytes_read);

    return fres;
}

FRESULT WaveFile::close() {

    FRESULT fres;

    if (mode & FA_READ) {

        fres = f_close(fil);

    } else {

        fres = write_tags();

        if (fres == FR_OK) {
            fres = update_header();
            if (fres == FR_OK) {
                fres = f_close(fil);
            }
        }
    }

    mode = 0;
    return fres;
}

FRESULT WaveFile::write(char *p, uint32_t count) {

    FRESULT fres;
    UINT bytes_written;

    fres = f_write(fil, p, count, &bytes_written);

    data_size += bytes_written;

    return fres;
}

FRESULT WaveFile::update_header() {

    FRESULT fres;
    UINT bytes_written;

    header = {info.sample_rate, info.n_channels, static_cast<uint32_t>(data_size), static_cast<uint32_t>(tags_size)};

    const auto curr_fpos = fil->fptr;

    fres = f_lseek(fil, 0);

    if (fres == FR_OK) {
        fres = f_write(fil, &header, sizeof(header), &bytes_written);

        if (fres == FR_OK) {
            // Flush the file to start writing with full multi-sectors writes
            // If we don't flush it, there would be a partially written sector in FatFS and, when we write
            // the FIFO (say, 8 sectors), the writes would go like this:
            // 1 sector: when the partially written sector is filled
            // 7 sectors: the rest, using multi-sector write
            // But the FatFS buffer will be filled with some bytes, and the cycle will repeat (1-7-1-7....)
            fres = f_sync(fil);

            if (fres == FR_OK && curr_fpos) {
                fres = f_lseek(fil, curr_fpos);
            }
        }
    }

    return fres;
}

FRESULT WaveFile::write_tags() {

    FRESULT fres;
    UINT bytes_written;

    /*
     * Write the carrier frequency as a tag
     */
    char buf[64];
    snprintf(buf, 64, "%lu", (uint32_t)info.carrier_freq);
    tags_t tags{buf};

    fres = f_write(fil, &tags, sizeof(tags), &bytes_written);

    tags_size = sizeof(tags);

    return fres;
}
