//
// Created by Angel Dust on 15/06/2025
//

#ifndef __FILE_H__
#define __FILE_H__

#include "ff.h"
#include "result.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <array>
#include <memory>
#include <iterator>
#include <vector>

namespace io {

struct filesystem_error {
    constexpr filesystem_error() = default;

    constexpr filesystem_error(FRESULT fatfs_error) : err{fatfs_error} {
    }

    constexpr filesystem_error(unsigned int other_error) : err{other_error} {
    }

    uint32_t code() const {
        return err;
    }

    std::string what() const;

    bool ok() const {
        return err == FR_OK;
    }

  private:
    uint32_t err{FR_OK};
};

struct path {

    using string_type = std::string; // std::u16string;
    using value_type = string_type::value_type;

    static constexpr value_type preferred_separator = '/'; // u'/';

    path() : _s{} {
    }

    path(const path &p) : _s{p._s} {
    }

    path(path &&p) : _s{std::move(p._s)} {
    }

    template <class Source> path(const Source &source) : path{std::begin(source), std::end(source)} {
    }

    template <class InputIt> path(InputIt first, InputIt last) : _s{first, last} {
    }

    // path(const value_t *const s) : _s{s} {
    // }

    path(const TCHAR *const s) : _s{reinterpret_cast<const io::path::value_type *>(s)} {
    }

    path &operator=(const path &p) {
        _s = p._s;
        return *this;
    }

    path &operator=(path &&p) {
        _s = std::move(p._s);
        return *this;
    }

    path parent_path() const;
    path extension() const;
    path filename() const;
    path stem() const;

    bool empty() const {
        return _s.empty();
    }

    const value_type *c_str() const {
        return native().c_str();
    }

    const TCHAR *tchar() const {
        return reinterpret_cast<const TCHAR *>(native().c_str());
    }

    const string_type &native() const {
        return _s;
    }

    std::string string() const;

    path &operator+=(const path &p) {
        _s += p._s;
        return *this;
    }

    path &operator+=(const string_type &str) {
        _s += str;
        return *this;
    }

    path &operator/=(const path &p) {
        if (_s.back() != preferred_separator && p._s.front() != preferred_separator) {
            _s += preferred_separator;
        }
        _s += p._s;
        return *this;
    }

    path &replace_extension(const path &replacement = path());

    path &append_filename(const string_type &str);

  private:
    string_type _s;
};

bool operator==(const path &lhs, const path &rhs);
bool operator!=(const path &lhs, const path &rhs);
bool operator<(const path &lhs, const path &rhs);
bool operator>(const path &lhs, const path &rhs);
path operator+(const path &lhs, const path &rhs);
path operator/(const path &lhs, const path &rhs);

/* Case insensitive path equality on underlying "native" string. */
bool path_iequal(const path &lhs, const path &rhs);
bool is_cxx_capture_file(const path &filename);
uint8_t capture_file_sample_size(const path &filename);

using file_status = BYTE;

/* The largest block that can be read/written to a file. */
constexpr uint16_t max_file_block_size = 512;

static_assert(sizeof(path::value_type) == 1, "sizeof(io::path::value_type) != 1");
static_assert(sizeof(path::value_type) == sizeof(TCHAR), "FatFs TCHAR size != io::path::value_type");

struct space_info {
    static_assert(sizeof(std::uintmax_t) >= 8, "std::uintmax_t too small (<uint64_t)");

    std::uintmax_t capacity;
    std::uintmax_t free;
    std::uintmax_t available;
};

struct directory_entry : public FILINFO {
    file_status status() const {
        return fattrib;
    }

    std::uintmax_t size() const {
        return fsize;
    };

    const io::path path() const noexcept {
        return {fname};
    };
};

class directory_iterator {
    struct Impl {
        DIR dir;
        directory_entry filinfo;

        ~Impl() {
            f_closedir(&dir);
        }
    };

    std::shared_ptr<Impl> impl{};
    io::path path_{};
    io::path wild_{};

    friend bool operator!=(const directory_iterator &lhs, const directory_iterator &rhs);

  public:
    using difference_type = std::ptrdiff_t;
    using value_type = directory_entry;
    using pointer = const directory_entry *;
    using reference = const directory_entry &;
    using iterator_category = std::input_iterator_tag;

    directory_iterator() noexcept {};
    directory_iterator(const io::path &path, const io::path &wild);

    ~directory_iterator() {
    }

    directory_iterator &operator++();

    reference operator*() const {
        // TODO: Exception or assert if impl == nullptr.
        return impl->filinfo;
    }
};

inline const directory_iterator &begin(const directory_iterator &iter) noexcept {
    return iter;
};
inline directory_iterator end(const directory_iterator &) noexcept {
    return {};
};

inline bool operator!=(const directory_iterator &lhs, const directory_iterator &rhs) {
    return lhs.impl != rhs.impl;
};

bool is_directory(const file_status s);
bool is_regular_file(const file_status s);
bool file_exists(const path &file_path);
bool is_directory(const path &file_path);
bool is_empty_directory(const path &file_path);

int file_count(const path &dir_path);

space_info space(const path &p);

} // namespace io

struct FATTimestamp {
    uint16_t FAT_date;
    uint16_t FAT_time;
};

io::filesystem_error delete_file(const io::path &file_path);
io::filesystem_error rename_file(const io::path &file_path, const io::path &new_name);
io::filesystem_error copy_file(const io::path &file_path, const io::path &dest_path);

FATTimestamp file_created_date(const io::path &file_path);
io::filesystem_error file_update_date(const io::path &file_path, FATTimestamp timestamp);
io::filesystem_error make_new_file(const io::path &file_path);
io::filesystem_error make_new_directory(const io::path &dir_path);
io::filesystem_error ensure_directory(const io::path &dir_path);

template <typename TCallback> void scan_root_files(const io::path &directory, const io::path &extension, const TCallback &fn) {
    for (const auto &entry : io::directory_iterator(directory, extension)) {
        if (io::is_regular_file(entry.status())) {
            fn(entry.path());
        }
    }
}
std::vector<io::path> scan_root_files(const io::path &directory, const io::path &extension);
std::vector<io::path> scan_root_directories(const io::path &directory);

/* Gets an auto incrementing filename stem.
 * Pattern should be like "FOO_???.txt" where ??? will be replaced by digits.
 * Pattern may also contain a folder path like "LOGS/FOO_???.txt".
 * Pattern '?' must be contiguous (bad: "FOO?_??")
 * Returns empty path if a filename could not be created. */
io::path next_filename_matching_pattern(const io::path &pattern);

/* Values added to FatFs FRESULT enum, values outside the FRESULT data type */
static_assert(sizeof(FIL::err) == 1, "FatFs FIL::err size not expected.");

/* Dangerous to expose these, as FatFs native error values are byte-sized. However,
 * my filesystem_error implementation is fine with it. */
#define FR_DISK_FULL (0x100)
#define FR_EOF (0x101)
#define FR_BAD_SEEK (0x102)
#define FR_UNEXPECTED (0x103)

/* NOTE: sizeof(File) == 556 bytes because of the FIL's buf member. */
class FatFSFile {
  public:
    using Size = uint64_t;
    using Offset = uint64_t;
    using Timestamp = uint32_t;
    using Error = io::filesystem_error;

    template <typename T> using Result = Result<T, Error>;

    FatFSFile(){};
    ~FatFSFile();

    FatFSFile(FatFSFile &&other) {
        std::swap(f, other.f);
    }
    FatFSFile &operator=(FatFSFile &&other) {
        std::swap(f, other.f);
        return *this;
    }

    /* Prevent copies */
    FatFSFile(const FatFSFile &) = delete;
    FatFSFile &operator=(const FatFSFile &) = delete;

    // TODO: Return Result<>.
    io::filesystem_error open(const io::path &filename, bool read_only = true, bool create = false);
    void close();
    io::filesystem_error append(const io::path &filename);
    io::filesystem_error create(const io::path &filename);

    Result<Size> read(void *data, const Size bytes_to_read);
    Result<Size> write(const void *data, Size bytes_to_write);

    Offset tell() const;
    Result<Offset> seek(uint64_t Offset);
    Result<Offset> truncate();
    Size size() const;
    Result<bool> eof();

    template <size_t N> Result<Size> write(const std::array<uint8_t, N> &data) {
        return write(data.data(), N);
    }

    io::filesystem_error write_line(const std::string &s);

    io::filesystem_error sync();

  private:
    FIL f{};

    io::filesystem_error open_fatfs(const io::path &filename, BYTE mode);
};

#endif /*__FILE_H__*/
