//
// Created by Angel Dust on 15/06/2025
//

#include "fatfs_file.h"
#include "fatfs/fatfs.h"
#include "ff.h"
#include <algorithm>
#include <codecvt>
#include <cstring>
#include <locale>
#include "status.h"

io::filesystem_error FatFSFile::open_fatfs(const io::path &filename, BYTE mode) {
    lock_sd_card();
    auto result = f_open(&f, reinterpret_cast<const TCHAR *>(filename.c_str()), mode);
    if (result == FR_OK) {
        if (mode & FA_OPEN_ALWAYS) {
            const auto result = f_lseek(&f, f_size(&f));
            if (result != FR_OK) {
                f_close(&f);
            }
        }
    }

    unlock_sd_card();
    if (result == FR_OK) {
        return {};
    } else {
        return {result};
    }
}

/*
 * @param read_only: open in readonly mode
 * @param create: create if it doesnt exist
 */
io::filesystem_error FatFSFile::open(const io::path &filename, bool read_only, bool create) {
    BYTE mode = read_only ? FA_READ : FA_READ | FA_WRITE;
    if (create) {
        mode |= FA_OPEN_ALWAYS;
    }

    return open_fatfs(filename, mode);
}

io::filesystem_error FatFSFile::append(const io::path &filename) {
    return open_fatfs(filename, FA_WRITE | FA_OPEN_ALWAYS);
}

io::filesystem_error FatFSFile::create(const io::path &filename) {
    return open_fatfs(filename, FA_WRITE | FA_CREATE_ALWAYS);
}

FatFSFile::~FatFSFile() {
    lock_sd_card();
    f_close(&f);
    unlock_sd_card();
}

void FatFSFile::close() {
    f_close(&f);
}

FatFSFile::Result<FatFSFile::Size> FatFSFile::read(void *data, Size bytes_to_read) {
    UINT bytes_read = 0;
    lock_sd_card();
    const auto result = f_read(&f, data, bytes_to_read, &bytes_read);
    unlock_sd_card();
    if (result == FR_OK) {
        return {static_cast<size_t>(bytes_read)};
    } else {
        return {static_cast<Error>(result)};
    }
}

FatFSFile::Result<FatFSFile::Size> FatFSFile::write(const void *data, Size bytes_to_write) {
    UINT bytes_written = 0;
    lock_sd_card();
    const auto result = f_write(&f, data, bytes_to_write, &bytes_written);
    unlock_sd_card();
    if (result == FR_OK) {
        if (bytes_to_write == bytes_written) {
            return {static_cast<FatFSFile::Size>(bytes_written)};
        } else {
            return Error{FR_DISK_FULL};
        }
    } else {
        return {static_cast<Error>(result)};
    }
}

FatFSFile::Offset FatFSFile::tell() const {
    return f_tell(&f);
}

bool FatFSFile::eof() {
    return f_eof(&f);
}

FatFSFile::Result<FatFSFile::Offset> FatFSFile::seek(Offset new_position) {
    /* NOTE: Returns *old* position, not new position */
    const auto old_position = tell();
    if (!lock_sd_card()) {
        return FR_LOCKED;
    };
    const auto result = f_lseek(&f, new_position);
    unlock_sd_card();
    if (result != FR_OK) {
        return {static_cast<Error>(result)};
    }
    if (f_tell(&f) != new_position) {
        return {static_cast<Error>(FR_BAD_SEEK)};
    }
    return {static_cast<FatFSFile::Offset>(old_position)};
}

FatFSFile::Result<bool> FatFSFile::ready(uint16_t timeout_ms) {
    if (!lock_sd_card(timeout_ms)) {
        return {static_cast<Error>(FR_LOCKED)};
    };
    unlock_sd_card();

    return true;
}

FatFSFile::Result<FatFSFile::Offset> FatFSFile::truncate() {
    const auto position = f_tell(&f);
    if (!lock_sd_card()) {
        return FR_LOCKED;
    };
    auto result = f_truncate(&f);
    result = result == FR_OK ? f_sync(&f) : result;
    unlock_sd_card();
    if (result != FR_OK) {
        return {static_cast<Error>(result)};
    }
    return {static_cast<FatFSFile::Offset>(position)};
}

FatFSFile::Size FatFSFile::size() const {
    return f_size(&f);
}

io::filesystem_error FatFSFile::write_line(const std::string &s) {
    const auto result_s = write(s.c_str(), s.size());
    if (result_s.is_error()) {
        return {result_s.error()};
    }

    const auto result_crlf = write("\r\n", 2);
    if (result_crlf.is_error()) {
        return {result_crlf.error()};
    }

    return {};
}

io::filesystem_error FatFSFile::sync() {
    if (!lock_sd_card()) {
        return FR_LOCKED;
    };
    const auto result = f_sync(&f);
    unlock_sd_card();
    if (result == FR_OK) {
        return {};
    } else {
        return {result};
    }
}

/* Range used for filename matching.
 * Start and end are inclusive positions of "???" */
struct pattern_range {
    size_t start;
    size_t end;
};

/* Finds the last file matching the specified pattern that
 * can be automatically incremented (digits in pattern).
 * NB: assumes a patten with contiguous '?' like "FOO_???.txt". */
static io::path find_last_ordinal_match(const io::path &folder, const io::path &pattern, pattern_range range) {
    auto last_match = io::path();
    auto can_increment = [range](const auto &path) {
        for (auto i = range.start; i <= range.end; ++i) {
            if (!isdigit(path.native()[i])) {
                return false;
            }
        }

        return true;
    };

    for (const auto &entry : io::directory_iterator(folder, pattern)) {
        if (io::is_regular_file(entry.status()) && can_increment(entry.path())) {
            const auto &match = entry.path();
            if (match > last_match) {
                last_match = match;
            }
        }
    }

    return last_match;
}

/* Given a file path like "FOO_0001.txt" increment it to "FOO_0002.txt". */
static io::path increment_filename_ordinal(const io::path &path, pattern_range range) {
    auto name = path.filename().native();

    for (auto i = range.end; i >= range.start; --i) {
        auto &c = name[i];

        // Not a digit or would overflow the counter.
        if (c < '0' || c > '9' || (c == '9' && i == range.start)) {
            return {};
        }

        if (c == '9') {
            c = '0';
        } else {
            c++;
            break;
        }
    }

    return {name};
}

io::path next_filename_matching_pattern(const io::path &filename_pattern) {
    auto path = filename_pattern.parent_path();
    auto pattern = filename_pattern.filename();
    auto range = pattern_range{pattern.native().find_first_of('?'), pattern.native().find_last_of('?')};

    const auto match = find_last_ordinal_match(path, pattern, range);

    if (match.empty()) {
        auto pattern_str = pattern.native();
        for (auto i = range.start; i <= range.end; ++i) {
            pattern_str[i] = '0';
        }
        return path / pattern_str;
    }

    auto next_name = increment_filename_ordinal(match, range);
    return next_name.empty() ? next_name : path / next_name;
}

std::vector<io::path> scan_root_files(const io::path &directory, const io::path &extension) {
    std::vector<io::path> file_list{};
    scan_root_files(directory, extension, [&file_list](const io::path &p) {
        file_list.push_back(p);
    });

    return file_list;
}

std::vector<io::path> scan_root_directories(const io::path &directory) {
    std::vector<io::path> directory_list{};

    for (const auto &entry : io::directory_iterator(directory, "*")) {
        if (io::is_directory(entry.status())) {
            directory_list.push_back(entry.path());
        }
    }

    return directory_list;
}

io::filesystem_error delete_file(const io::path &file_path) {
    lock_sd_card();
    FRESULT res = f_unlink(reinterpret_cast<const TCHAR *>(file_path.c_str()));
    unlock_sd_card();
    return {res};
}

io::filesystem_error rename_file(const io::path &file_path, const io::path &new_name) {
    lock_sd_card();
    FRESULT res = f_rename(reinterpret_cast<const TCHAR *>(file_path.c_str()), reinterpret_cast<const TCHAR *>(new_name.c_str()));
    unlock_sd_card();
    return {res};
}

io::filesystem_error copy_file(const io::path &file_path, const io::path &dest_path) {
    constexpr size_t buffer_size = io::max_file_block_size;
    uint8_t buffer[buffer_size];
    FatFSFile src;
    FatFSFile dst;

    auto error = src.open(file_path);
    if (!error.ok()) {
        return error;
    }

    error = dst.create(dest_path);
    if (!error.ok()) {
        return error;
    }

    while (true) {
        auto result = src.read(buffer, buffer_size);
        if (result.is_error()) {
            return result.error();
        }

        result = dst.write(buffer, *result);
        if (result.is_error()) {
            return result.error();
        }

        if (*result < buffer_size) {
            break;
        }
    }

    return {};
}

FATTimestamp file_created_date(const io::path &file_path) {
    FILINFO filinfo;

    lock_sd_card();
    f_stat(reinterpret_cast<const TCHAR *>(file_path.c_str()), &filinfo);
    unlock_sd_card();
    return {filinfo.fdate, filinfo.ftime};
}

io::filesystem_error file_update_date(const io::path &file_path, FATTimestamp timestamp) {
    FILINFO filinfo{};

    filinfo.fdate = timestamp.FAT_date;
    filinfo.ftime = timestamp.FAT_time;
    lock_sd_card();
    return f_utime(reinterpret_cast<const TCHAR *>(file_path.c_str()), &filinfo);
    unlock_sd_card();
}

io::filesystem_error make_new_file(const io::path &file_path) {
    FatFSFile f;
    auto error = f.create(file_path);
    if (!error.ok()) {
        return error;
    }

    return {};
}

io::filesystem_error make_new_directory(const io::path &dir_path) {
    lock_sd_card();
    FRESULT res = f_mkdir(reinterpret_cast<const TCHAR *>(dir_path.c_str()));
    unlock_sd_card();
    return {res};
}

io::filesystem_error ensure_directory(const io::path &dir_path) {
    if (dir_path.empty() || io::file_exists(dir_path)) {
        return {};
    }

    auto result = ensure_directory(dir_path.parent_path());
    if (result.code()) {
        return result;
    }

    return make_new_directory(dir_path);
}

namespace io {

std::string filesystem_error::what() const {
    switch (err) {
        case FR_OK:
            return "ok";
        case FR_DISK_ERR:
            return "disk error";
        case FR_INT_ERR:
            return "insanity detected";
        case FR_NOT_READY:
            return "SD card not ready";
        case FR_NO_FILE:
            return "no file";
        case FR_NO_PATH:
            return "no path";
        case FR_INVALID_NAME:
            return "invalid name";
        case FR_DENIED:
            return "denied";
        case FR_EXIST:
            return "exists";
        case FR_INVALID_OBJECT:
            return "invalid object";
        case FR_WRITE_PROTECTED:
            return "write protected";
        case FR_INVALID_DRIVE:
            return "invalid drive";
        case FR_NOT_ENABLED:
            return "not enabled";
        case FR_NO_FILESYSTEM:
            return "no filesystem";
        case FR_MKFS_ABORTED:
            return "mkfs aborted";
        case FR_TIMEOUT:
            return "timeout";
        case FR_LOCKED:
            return "locked";
        case FR_NOT_ENOUGH_CORE:
            return "not enough core";
        case FR_TOO_MANY_OPEN_FILES:
            return "too many open files";
        case FR_INVALID_PARAMETER:
            return "invalid parameter";
        case FR_EOF:
            return "end of file";
        case FR_DISK_FULL:
            return "disk full";
        case FR_BAD_SEEK:
            return "bad seek";
        case FR_UNEXPECTED:
            return "unexpected";
        default:
            return "unknown";
    }
}

bool path_iequal(const path &lhs, const path &rhs) {
    const auto &lhs_str = lhs.native();
    const auto &rhs_str = rhs.native();

    // NB: Not correct for Unicode/locales.
    if (lhs_str.length() == rhs_str.length()) {
        for (size_t i = 0; i < lhs_str.length(); ++i) {
            if (towupper(lhs_str[i]) != towupper(rhs_str[i])) {
                return false;
            }
        }

        return true;
    }

    return false;
}

directory_iterator::directory_iterator(const io::path &path, const io::path &wild) : path_{path}, wild_{wild} {
    impl = std::make_shared<Impl>();
    auto result = f_findfirst(&impl->dir, &impl->filinfo, path_.tchar(), wild_.tchar());
    if (result != FR_OK || impl->filinfo.fname[0] == (TCHAR)'\0') {
        impl.reset();
        // TODO: Throw exception if/when I enable exceptions...
    }
}

directory_iterator &directory_iterator::operator++() {
    const auto result = f_findnext(&impl->dir, &impl->filinfo);
    if ((result != FR_OK) || (impl->filinfo.fname[0] == 0)) {
        impl.reset();
    }
    return *this;
}

bool is_directory(const file_status s) {
    return (s & AM_DIR);
}

bool is_regular_file(const file_status s) {
    return !(s & AM_DIR);
}

bool file_exists(const path &file_path) {
    FILINFO filinfo;
    lock_sd_card();
    auto fr = f_stat(reinterpret_cast<const TCHAR *>(file_path.c_str()), &filinfo);
    unlock_sd_card();
    return fr == FR_OK;
}

bool is_directory(const path &file_path) {
    FILINFO filinfo;
    lock_sd_card();
    auto fr = f_stat(reinterpret_cast<const TCHAR *>(file_path.c_str()), &filinfo);
    unlock_sd_card();

    return fr == FR_OK && is_directory(static_cast<file_status>(filinfo.fattrib));
}

FRESULT check_and_create_folder(const char *path) {
    FRESULT res;
    FILINFO fno;
    lock_sd_card();
    // Check if folder exists
    res = f_stat(path, &fno);

    if (res == FR_OK) {
        // Path exists, check if it's a directory
        if (fno.fattrib & AM_DIR) {
            res = FR_OK; // Folder exists
        } else {
            res = FR_EXIST; // Path exists but it's a file, not a folder
        }
    } else if (res == FR_NO_FILE) {
        // Folder doesn't exist, create it
        res = f_mkdir(path);
    }

    unlock_sd_card();
    return res;
}

bool is_empty_directory(const path &file_path) {
    DIR dir;
    FILINFO filinfo;

    if (!is_directory(file_path)) {
        return false;
    }

    lock_sd_card();
    auto result = f_findfirst(&dir, &filinfo, reinterpret_cast<const TCHAR *>(file_path.c_str()), (const TCHAR *)"*");
    unlock_sd_card();
    return !((result == FR_OK) && (filinfo.fname[0] != (TCHAR)'\0'));
}

int file_count(const path &directory) {
    int count{0};

    for (auto &entry : io::directory_iterator(directory, (const TCHAR *)"*")) {
        (void)entry; // avoid unused warning
        ++count;
    }

    return count;
}

space_info space(const path &p) {
    DWORD free_clusters{0};
    FATFS *fs;
    lock_sd_card();
    FRESULT res = f_getfree(reinterpret_cast<const TCHAR *>(p.c_str()), &free_clusters, &fs);
    unlock_sd_card();
    if (res == FR_OK) {
#if _MAX_SS != _MIN_SS
        static_assert(false, "FatFs not configured for fixed sector size");
#else
        const std::uintmax_t cluster_bytes = fs->csize * _MIN_SS;
        return {
            (fs->n_fatent - 2) * cluster_bytes,
            free_clusters * cluster_bytes,
            free_clusters * cluster_bytes,
        };
#endif
    } else {
        return {0, 0, 0};
    }
}

} /* namespace io */
