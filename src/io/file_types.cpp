//
// Created by Angel Dust on 14/12/2022.
//

#include "file_types.h"
#include "fatfs_file.h"
#include "stdlib.h"
#include "string.h"

const char *file_type_names[] = {"wav", "cs16"};
const char *file_type_extensions[] = {"wav", "cs16"};

FileType get_file_type_from_extension(const io::path &path) {

    FileType ftype = FTYPE_UNKNOWN;

    char *point;

    if ((point = strrchr(path.c_str(), '.')) != NULL) {
        for (int i = 0; i != FTYPE_LAST; i++) {
            if (strcasecmp(point + 1, file_type_extensions[i]) == 0) {
                ftype = (FileType)i;
                break;
            }
        }
    }

    return ftype;
}

WaveInfo get_info_from_file_path(const io::path &path) {
    WaveInfo wi = {};

    if (path.empty()) {
        return wi;
    }

    char const *p = path.c_str();
    while (*p) {
        if (*p >= '0' && *p <= '9') {
            char const *n = p; // number starts here
            while (*p >= '0' && *p <= '9') {
                ++p;
            }
            if (*p == '.') {
                ++p;
                // if not [0-9] after '.' abort
                if (*p < '0' || *p > '9') {
                    continue;
                }
                while (*p >= '0' && *p <= '9') {
                    ++p;
                }
            }
            char const *s = p; // number ends and unit starts here
            while ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z')) {
                ++p;
            }
            double num = atof(n);
            size_t len = p - s;
            double scale = 1.0;
            switch (*s) {
                case 'k':
                case 'K':
                    scale *= 1e3;
                    break;
                case 'M':
                case 'm':
                    scale *= 1e6;
                    break;
                case 'G':
                case 'g':
                    scale *= 1e9;
                    break;
            }
            if (len == 1 && !strncasecmp("M", s, 1))
                wi.carrier_freq = num * 1e6;
            else if (len == 1 && !strncasecmp("k", s, 1))
                wi.sample_rate = num * 1e3;
            else if (len == 2 && !strncasecmp("Hz", s, 2))
                wi.carrier_freq = num;
            else if (len == 3 && !strncasecmp("sps", s, 3))
                wi.sample_rate = num;
            else if (len == 3 && !strncasecmp("Hz", s + 1, 2) && scale > 1.0)
                wi.carrier_freq = num * scale;
            else if (len == 4 && !strncasecmp("sps", s + 1, 3) && scale > 1.0)
                wi.sample_rate = num * scale;

        } else {
            p++; // skip non-alphanum char otherwise
        }
    }

    return wi;
}
