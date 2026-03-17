//
// Created by Angel Dust on 18/16/2025.
//
#ifndef FILE_WRAPPER_HPP
#define FILE_WRAPPER_HPP

#include "dsp/decimation/dsp_fir_decimator_float.h"
#include "ff.h"
#include "printf.h"
#include "ring_buffer.hpp"

#include "status.h"
#include <cstdio>
#include <string>
#include <string_view>
#include <cstring>

#include "io/fatfs_file.h"

namespace io {

static constexpr uint32_t BUFFER_SIZE = 256;

enum FindMode { LTE, GTE, GT, LT, EQ };

template <uint32_t LINE_CACHE_SIZE = 16, uint32_t NEWLINE_CACHE_SIZE = 128> class FileWrapper {
  private:
    FatFSFile file_;

    struct CachedLine {
        uint32_t line_number;
        std::string content;
        uint32_t last_access_time;
        bool valid;

        CachedLine() : line_number(0), last_access_time(0), valid(false) {
        }
        void invalidate() {
            valid = false;
            content.clear();
            last_access_time = 0;
        }
    };

    struct NewlineEntry {
        uint32_t line_number;  // Which line this entry represents
        uint32_t start_offset; // File offset where the line starts
        uint32_t end_offset;   // File offset of the newline character (line end)

        NewlineEntry() : line_number(0), start_offset(0), end_offset(0) {
        }

        NewlineEntry(uint32_t line, uint32_t start, uint32_t end) : line_number(line), start_offset(start), end_offset(end) {
        }

        // Convenience method to get line length (excluding newline)
        uint32_t length() const {
            return (end_offset > start_offset) ? end_offset - start_offset : 0;
        }
    };

    CachedLine line_cache_[LINE_CACHE_SIZE];
    RingBuffer<NewlineEntry, NEWLINE_CACHE_SIZE> newline_cache_;

    uint32_t total_lines_{0};    // Total lines in file
    uint32_t access_counter_{0}; // For LRU tracking
    uint32_t cache_misses_{0};
    uint32_t cache_hits_{0};
    uint32_t cache_start_line_{0}; // First line number in newline cache
    io::path path_;

    char work_buffer_[BUFFER_SIZE]; // Shared work buffer

  public:
    FileWrapper() : total_lines_(0), access_counter_(0), cache_start_line_(0) {
    }

    bool load(const io::path &path, bool create) {

        file_.close();
        path_ = path;
        auto res = file_.open(path, false, create);

        if (res.ok()) {

            invalidate_all_caches();

            scan_file();

            // for (int i = 0; i < newline_cache_.size(); i++) {
            //  LOG("lc:%d:%d:%d\n", i, newline_cache_[i].offset, newline_cache_[i].line_number);
            //}
            return true;
        }

        return false;
    }

    ~FileWrapper() {
        file_.close();
    }

    FatFSFile *get_file() {
        return &file_;
    }

    uint32_t get_cache_size() {
        return LINE_CACHE_SIZE;
    }

    // Basic operations
    uint32_t line_count() const {
        return total_lines_;
    }
    uint32_t file_size() const {
        return file_.size();
    }
    uint32_t get_cache_misses() const {
        return cache_misses_;
    }
    uint32_t get_cache_hits() const {
        return cache_hits_;
    }

    // Get line content (uses cache with LRU)
    std::string get_line(uint32_t line_number);

    // Get line content by reference (cache-backed, avoids heap churn/copies).
    // The returned reference remains valid until the line is evicted from the cache.
    const std::string &get_line_ref(uint32_t line_number);

    // Prefetch lines starting from a given line
    void prefetch(uint32_t from_line, uint32_t count = LINE_CACHE_SIZE);

    // Modify operations (in-place editing)
    void replace_line(uint32_t line_number, const std::string &content);
    void insert_line(uint32_t line_number, const std::string &content);
    void append_line(const std::string &content);
    void delete_line(uint32_t line_number);

    void log();

    // Generic range queries for sorted data
    template <typename T, typename ExtractKey>
    FRESULT find_range(const T &min_key, const T &max_key, ExtractKey extract_key, std::vector<uint32_t> &results, uint32_t max = 0);

    template <typename T, typename ExtractKey>
    Result<uint32_t, io::filesystem_error> binary_search_first(const T &key, ExtractKey extract_key, FindMode mode = GTE);

    // Bulk operations
    std::vector<std::string> get_lines_range(uint32_t start_line, uint32_t end_line);

    void refresh() {
        invalidate_all_caches();
        scan_file();
    }

    void clear() {
        file_.seek(0);
        file_.truncate();
        file_.sync();
        refresh();
    }

    // Create rotating backup of the file
    bool backup(uint8_t max_files = 2) {
        if (max_files == 0 || path_.empty()) {
            return false;
        }

        io::path base_name = path_.filename();
        io::path extension = path_.extension();

        // Rotate existing backups (from max_files down to 1)
        for (uint8_t i = max_files; i > 1; --i) {
            io::path current_backup = base_name + "_backup_" + std::to_string(i - 1) + extension;
            io::path next_backup = base_name + "_backup_" + std::to_string(i) + extension;

            // Check if current backup exists and rename it
            FatFSFile test_file;
            auto result = test_file.open(current_backup.c_str(), true, false);
            if (result.ok()) {
                test_file.close();
                f_unlink(next_backup.c_str()); // Remove target if exists
                f_rename(current_backup.c_str(), next_backup.c_str());
            }
        }

        // Create new backup_1
        io::path new_backup = base_name + "_backup_1" + extension;
        return copy_file_contents(path_.c_str(), new_backup.c_str());
    }

  private:
    void invalidate_all_caches();
    void scan_file();

    // Line cache management with LRU
    std::string *find_cached_line(uint32_t line_number);
    void cache_line(uint32_t line_number, const std::string &content);
    std::string &cache_line_move(uint32_t line_number, std::string &&content);
    uint32_t find_lru_cache_slot();
    void invalidate_lines_from(uint32_t line_number);

    // File structure operations using RingBuffer
    std::string read_line_from_file(uint32_t line_number);
    std::string_view read_line_view(uint32_t line_number, char *buf, size_t buf_size);
    uint32_t find_line_start_offset(uint32_t line_number);
    uint32_t find_line_end_offset(uint32_t line_number);
    void ensure_newline_cache_covers(uint32_t line_number);
    void update_newline_cache_after_edit(uint32_t from_offset, int32_t size_delta);

    // Low-level file manipulation
    void shift_file_content_right(uint32_t from_offset, uint32_t shift_amount);
    void shift_file_content_left(uint32_t from_offset, uint32_t shift_amount);
    void ensure_newline_at_eof();

    void fill_cache_from_offset(uint32_t start_offset, uint32_t start_line);

    void rebuild_cache(uint32_t start_line, uint32_t limit_offset = UINT32_MAX);

    uint32_t scan_to_line(uint32_t target_line, uint32_t limit_offset = UINT32_MAX);

    bool copy_file_contents(const char *src_path, const char *dst_path);
};

template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE>
bool FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::copy_file_contents(const char *src_path, const char *dst_path) {
    FatFSFile src_file, dst_file;

    auto src_result = src_file.open(src_path, true, false);
    if (!src_result.ok()) {
        return false;
    }

    auto dst_result = dst_file.open(dst_path, false, true);
    if (!dst_result.ok()) {
        src_file.close();
        return false;
    }

    char copy_buffer[BUFFER_SIZE];

    while (true) {
        auto read_result = src_file.read(copy_buffer, BUFFER_SIZE);
        if (read_result.is_error() || *read_result == 0) {
            break;
        }

        auto write_result = dst_file.write(copy_buffer, *read_result);
        if (write_result.is_error()) {
            src_file.close();
            dst_file.close();
            return false;
        }

        if (*read_result < BUFFER_SIZE) {
            break;
        }
    }

    src_file.close();
    dst_file.close();
    return true;
}

// Generic binary search implementation
template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE>
template <typename T, typename ExtractKey>
Result<uint32_t, io::filesystem_error> FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::binary_search_first(const T &key, ExtractKey extract_key,
                                                                                                             FindMode mode) {

    uint32_t left = 0;
    uint32_t right = total_lines_;
    uint32_t result = total_lines_;

    // LOG("** binary search: %d, mode:%u\n", key, (int)mode);

    while (left < right) {

        uint32_t mid = left + (right - left) / 2;

        // Use the cache-backed path for correctness.
        // With small caches and move-into-cache, heap churn is limited.
        const std::string &line = get_line_ref(mid);

        // LOG("left:%d,right:%d,mid:%d | line:%s | ", left, right, mid, line.c_str());
        if (line.empty()) {
            // LOG_RAW("empty key!!\n");
            return io::filesystem_error{FR_INT_ERR};
        }

        T line_key = extract_key(line);

        // LOG_RAW("key: %d | ", line_key);

        if (mode == EQ) {
            if (line_key == key) {

                result = mid;
                right = mid; // Continue searching left for first occurrence
                // LOG_RAW("condition TRUE (EQ) right=mid, result=mid -> %d\n", right);
            } else if (line_key < key) {

                left = mid + 1;
                // LOG_RAW("condition FALSE (EQ) left=mid+1 -> %d\n", left);
            } else {
                right = mid;
                // LOG_RAW("condition FALSE (EQ) right=mid -> %d\n", right);
            }
        } else {
            bool condition =
                (mode == LT && line_key < key) || (mode == LTE && line_key <= key) || (mode == GTE && line_key >= key) || (mode == GT && line_key > key);

            if (condition) {

                result = mid;
                if (mode == LT || mode == LTE) {

                    left = mid + 1; // Search right for last occurrence
                    // LOG_RAW("condition TRUE (lt,lte) left -> %d\n", left);
                } else {
                    right = mid; // Search left for first occurrence
                    // LOG_RAW("condition TRUE (gt,gte) right -> %d\n", right);
                }
            } else {
                if (mode == LT || mode == LTE) {

                    right = mid;
                    // LOG_RAW("condition FALSE (lt,lte) rigth -> %d\n", right);
                } else {
                    left = mid + 1;
                    // LOG_RAW("condition FALSE (gt,gte) left -> %d\n", left);
                }
            }
        }
    }

    // LOG("Result:%d\n", result);
    return result;
}

// Generic range search implementation
template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE>
template <typename T, typename ExtractKey>
FRESULT FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::find_range(const T &min_key, const T &max_key, ExtractKey extract_key, std::vector<uint32_t> &results,
                                                                     uint32_t max) {

    auto ready = file_.ready();

    if (ready.is_error()) {
        return FR_LOCKED;
    }

    // Find first line >= min_key
    auto res = binary_search_first(min_key, extract_key, GTE);
    if (res.is_error()) {
        return FR_INT_ERR;
    }

    uint32_t start = *res;

    // Find last line <= max_key
    res = binary_search_first(max_key, extract_key, LTE);

    if (res.is_error()) {
        return FR_INT_ERR;
    }

    uint32_t end = *res;

    if (end >= total_lines_) { // Not found
        return {};
    }

    if (end >= start) {

        if (max) { // if max=0 there's no limit
            end = min2(end, start + max);
        }

        // Collect line numbers in range
        results.reserve(end - start + 1);
        for (uint32_t i = start; i <= end; ++i) {

            results.push_back(i);
        }
    } else {
        // TODO: Shouldn't happen!! (seen when inserting a station)
    }

    return FR_OK;
}

// Bulk line retrieval implementation
template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE>
std::vector<std::string> FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::get_lines_range(uint32_t start_line, uint32_t end_line) {

    std::vector<std::string> results;
    uint32_t actual_end = std::min(end_line, total_lines_);

    if (start_line >= actual_end) {
        return results;
    }

    // Prefetch for efficiency when possible
    uint32_t range_size = actual_end - start_line;
    if (range_size <= LINE_CACHE_SIZE) {
        prefetch(start_line, range_size);
    }

    results.reserve(range_size);
    for (uint32_t i = start_line; i < actual_end; ++i) {
        std::string line = get_line(i);
        if (!line.empty()) {
            results.push_back(line);
        }
    }

    return results;
}

template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE> void FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::invalidate_all_caches() {
    for (uint32_t i = 0; i < LINE_CACHE_SIZE; ++i) {
        line_cache_[i].invalidate();
    }

    newline_cache_.clear();
    total_lines_ = 0;
    access_counter_ = 0;
    cache_start_line_ = 0;
}

template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE> void FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::scan_file() {
    if (file_size() == 0) {
        total_lines_ = 0;
        return;
    }

    file_.seek(0);
    cache_start_line_ = 0;
    newline_cache_.clear();

    // Count lines as: number of '\n' + 1 if file doesn't end with '\n'.
    total_lines_ = 0;

    uint32_t offset = 0;
    uint32_t current_line = 0;
    uint32_t line_start = 0; // Track start of current line
    char last_char = '\0';

    while (offset < file_size()) {
        uint32_t to_read = std::min((uint32_t)BUFFER_SIZE, file_size() - offset);
        auto result = file_.read(work_buffer_, to_read);

        if (result.is_error() || *result == 0) {
            break;
        }

        for (uint32_t i = 0; i < *result; ++i) {
            last_char = work_buffer_[i];
            if (last_char == '\n') {
                // Store both start and end for this line
                if (!newline_cache_.isFull()) {
                    newline_cache_.push(NewlineEntry(current_line, line_start, offset + i));
                }

                total_lines_++;
                current_line++;
                line_start = offset + i + 1; // Next line starts after this newline
            }
        }

        offset += *result;
        if (*result < to_read) {
            break;
        }
    }

    // If the file doesn't end with a newline, there is one final unterminated line.
    if (last_char != '\n' && file_size()) {
        total_lines_++;
    }
}

template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE>
std::string *FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::find_cached_line(uint32_t line_number) {
    for (uint32_t i = 0; i < LINE_CACHE_SIZE; ++i) {
        if (line_cache_[i].valid && line_cache_[i].line_number == line_number) {
            // Update LRU access time
            line_cache_[i].last_access_time = ++access_counter_;
            return &line_cache_[i].content;
        }
    }
    return nullptr;
}

template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE> uint32_t FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::find_lru_cache_slot() {
    // Find invalid slot first
    for (uint32_t i = 0; i < LINE_CACHE_SIZE; ++i) {
        if (!line_cache_[i].valid) {
            return i;
        }
    }

    // Find LRU slot
    uint32_t lru_slot = 0;
    uint32_t oldest_time = line_cache_[0].last_access_time;

    for (uint32_t i = 1; i < LINE_CACHE_SIZE; ++i) {
        if (line_cache_[i].last_access_time < oldest_time) {
            oldest_time = line_cache_[i].last_access_time;
            lru_slot = i;
        }
    }

    return lru_slot;
}

template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE>
void FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::cache_line(uint32_t line_number, const std::string &content) {
    uint32_t slot = find_lru_cache_slot();

    line_cache_[slot].line_number = line_number;
    line_cache_[slot].content = content;
    line_cache_[slot].last_access_time = ++access_counter_;
    line_cache_[slot].valid = true;
}

template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE>
std::string &FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::cache_line_move(uint32_t line_number, std::string &&content) {
    uint32_t slot = find_lru_cache_slot();

    line_cache_[slot].line_number = line_number;
    line_cache_[slot].content = std::move(content);
    line_cache_[slot].last_access_time = ++access_counter_;
    line_cache_[slot].valid = true;

    return line_cache_[slot].content;
}

template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE>
void FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::invalidate_lines_from(uint32_t line_number) {
    for (uint32_t i = 0; i < LINE_CACHE_SIZE; ++i) {
        if (line_cache_[i].valid && line_cache_[i].line_number >= line_number) {
            line_cache_[i].invalidate();
        }
    }
}

// template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE>
// void FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::ensure_newline_cache_covers(uint32_t line_number) {
//     // Check if line is already covered by cache
//     if (!newline_cache_.empty()) {
//         uint32_t cache_end_line = cache_start_line_ + newline_cache_.size();
//         if (line_number >= cache_start_line_ && line_number < cache_end_line) {
//             return; // Already covered
//         }
//     }

//     // Need to rebuild cache around this line
//     newline_cache_.clear();

//     // Start scanning from beginning of file to find the target line
//     file_.seek(0);
//     uint32_t offset = 0;
//     uint32_t current_line = 0;
//     cache_start_line_ = 0;

//     // Scan until we reach the target line or fill the cache
//     while (offset < file_size() && !newline_cache_.isFull()) {
//         uint32_t to_read = std::min(BUFFER_SIZE, file_size() - offset);
//         auto result = file_.read(work_buffer_, to_read);

//         if (result.is_error() || *result == 0) {
//             break;
//         }

//         for (uint32_t i = 0; i < *result; ++i) {
//             if (work_buffer_[i] == '\n') {
//                 newline_cache_.push(NewlineEntry(current_line, offset + i));
//                 current_line++;

//                 if (newline_cache_.isFull()) {
//                     return;
//                 }
//             }
//         }

//         offset += *result;
//         if (*result < to_read) {
//             break;
//         }
//     }
// }

template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE>
void FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::ensure_newline_cache_covers(uint32_t line_number) {
    // Check if line is already covered by cache
    if (!newline_cache_.empty()) {
        uint32_t cache_end_line = cache_start_line_ + newline_cache_.size();
        if (line_number >= cache_start_line_ && line_number < cache_end_line) {
            return; // Already covered
        }
    }

    // Calculate optimal cache start position to center the requested line
    uint32_t cache_capacity = newline_cache_.capacity();
    uint32_t optimal_start;

    if (line_number >= cache_capacity / 2) {
        optimal_start = line_number - cache_capacity / 2;
    } else {
        optimal_start = 0;
    }

    // Ensure we don't go past the end of the file
    if (optimal_start + cache_capacity > total_lines_) {
        optimal_start = (total_lines_ > cache_capacity) ? total_lines_ - cache_capacity : 0;
    }

    // Check if we can slide the cache instead of rebuilding
    if (!newline_cache_.empty()) {
        uint32_t current_start = cache_start_line_;
        uint32_t current_end = cache_start_line_ + newline_cache_.size();

        // Can we slide forward efficiently?
        if (optimal_start > current_start && optimal_start < current_end) {
            uint32_t lines_to_skip = optimal_start - current_start;

            // Remove entries from front
            for (uint32_t i = 0; i < lines_to_skip && !newline_cache_.empty(); ++i) {
                newline_cache_.pop();
            }
            cache_start_line_ = optimal_start;

            // Fill the rest by continuing from where we left off
            if (!newline_cache_.empty()) {
                NewlineEntry &last_entry = newline_cache_[newline_cache_.size() - 1];
                fill_cache_from_offset(last_entry.end_offset + 1, last_entry.line_number + 1);
            }
            return;
        }

        // Can we use existing cache data to limit backward scanning?
        if (optimal_start < current_start && optimal_start + cache_capacity > current_start) {
            NewlineEntry &first_entry = newline_cache_[0];
            rebuild_cache(optimal_start, first_entry.start_offset);
            return;
        }
    }

    // No overlap - rebuild from scratch
    rebuild_cache(optimal_start, UINT32_MAX);
}

template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE>
void FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::fill_cache_from_offset(uint32_t start_offset, uint32_t start_line) {
    file_.seek(start_offset);
    uint32_t offset = start_offset;
    uint32_t current_line = start_line;
    uint32_t line_start = start_offset;

    while (offset < file_size() && !newline_cache_.isFull()) {
        uint32_t to_read = std::min(BUFFER_SIZE, file_size() - offset);
        auto result = file_.read(work_buffer_, to_read);

        if (result.is_error() || *result == 0) {
            break;
        }

        for (uint32_t i = 0; i < *result; ++i) {
            if (work_buffer_[i] == '\n') {
                newline_cache_.push(NewlineEntry(current_line, line_start, offset + i));
                current_line++;
                line_start = offset + i + 1;

                if (newline_cache_.isFull()) {
                    return;
                }
            }
        }

        offset += *result;
        if (*result < to_read) {
            break;
        }
    }
}

template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE>
void FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::rebuild_cache(uint32_t start_line, uint32_t limit_offset) {
    newline_cache_.clear();
    cache_start_line_ = start_line;

    if (start_line == 0) {
        // Start from beginning
        fill_cache_from_offset(0, 0);
    } else {
        // Find start offset for start_line
        uint32_t start_offset = scan_to_line(start_line, limit_offset);
        fill_cache_from_offset(start_offset, start_line);
    }
}

template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE>
uint32_t FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::scan_to_line(uint32_t target_line, uint32_t limit_offset) {
    file_.seek(0);
    uint32_t offset = 0;
    uint32_t current_line = 0;
    uint32_t line_start = 0;
    uint32_t max_offset = (limit_offset == UINT32_MAX) ? file_size() : std::min(file_size(), limit_offset);

    while (offset < max_offset && current_line < target_line) {
        uint32_t to_read = std::min(BUFFER_SIZE, max_offset - offset);
        auto result = file_.read(work_buffer_, to_read);

        if (result.is_error() || *result == 0) {
            break;
        }

        for (uint32_t i = 0; i < *result; ++i) {
            if (work_buffer_[i] == '\n') {
                current_line++;
                if (current_line == target_line) {
                    return offset + i + 1;
                }
                line_start = offset + i + 1;
            }
        }

        offset += *result;
        if (*result < to_read) {
            break;
        }
    }

    return line_start;
}

template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE>
uint32_t FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::find_line_start_offset(uint32_t line_number) {
    if (line_number == 0) {
        return 0;
    }

    // Try to find in cache first - now we can directly get the start!
    if (!newline_cache_.empty()) {
        uint32_t cache_end_line = cache_start_line_ + newline_cache_.size();
        if (line_number >= cache_start_line_ && line_number < cache_end_line) {
            uint32_t cache_index = line_number - cache_start_line_;
            return newline_cache_[cache_index].start_offset;
        }
    }

    // Fallback: ensure cache covers this line, then try again
    // LOG("ensure_newline_cache_covers %d\n", line_number);
    ensure_newline_cache_covers(line_number);
    // LOG("END ensure_newline_cache_covers %d\n", line_number);
    if (!newline_cache_.empty()) {
        uint32_t cache_end_line = cache_start_line_ + newline_cache_.size();
        if (line_number >= cache_start_line_ && line_number < cache_end_line) {
            uint32_t cache_index = line_number - cache_start_line_;
            return newline_cache_[cache_index].start_offset;
        }
    }

    // Final fallback: scan from beginning (should rarely happen)
    // LOG("Scan to line!!!!!\n");
    return scan_to_line(line_number);
}

template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE>
uint32_t FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::find_line_end_offset(uint32_t line_number) {
    // Try to find in cache first
    if (!newline_cache_.empty()) {
        uint32_t cache_end_line = cache_start_line_ + newline_cache_.size();
        if (line_number >= cache_start_line_ && line_number < cache_end_line) {
            uint32_t cache_index = line_number - cache_start_line_;
            return newline_cache_[cache_index].end_offset;
        }
    }

    // Fallback: ensure cache covers this line, then try again
    ensure_newline_cache_covers(line_number);

    if (!newline_cache_.empty()) {
        uint32_t cache_end_line = cache_start_line_ + newline_cache_.size();
        if (line_number >= cache_start_line_ && line_number < cache_end_line) {
            uint32_t cache_index = line_number - cache_start_line_;
            return newline_cache_[cache_index].end_offset;
        }
    }

    // Final fallback: scan from line start
    uint32_t start_offset = find_line_start_offset(line_number);
    file_.seek(start_offset);
    uint32_t offset = start_offset;

    while (offset < file_size()) {
        uint32_t to_read = std::min(BUFFER_SIZE, file_size() - offset);
        auto result = file_.read(work_buffer_, to_read);

        if (result.is_error() || *result == 0) {
            break;
        }

        for (uint32_t i = 0; i < *result; ++i) {
            if (work_buffer_[i] == '\n') {
                return offset + i;
            }
        }

        offset += *result;
        if (*result < to_read) {
            break;
        }
    }

    return file_size();
}

template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE>
std::string FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::read_line_from_file(uint32_t line_number) {

    if (line_number >= total_lines_) {
        return {};
    }

    uint32_t start_offset = find_line_start_offset(line_number);
    uint32_t end_offset = find_line_end_offset(line_number);

    if (start_offset >= end_offset) {
        return {};
    }

    uint32_t length = end_offset - start_offset;
    std::string result;
    result.reserve(length);

    file_.seek(start_offset);
    uint32_t remaining = length;

    while (remaining > 0) {
        uint32_t to_read = std::min(BUFFER_SIZE, remaining);
        auto read_result = file_.read(work_buffer_, to_read);

        if (read_result.is_error() || *read_result == 0) {
            break;
        }

        result.append(work_buffer_, *read_result);
        remaining -= *read_result;
    }
    // LOG("Reading line %d (%d,%d): %s\n", line_number, start_offset, end_offset, result.c_str());
    return result;
}

template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE>
std::string_view FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::read_line_view(uint32_t line_number, char *buf, size_t buf_size) {

    if (!buf || buf_size < 2) {
        return {};
    }

    if (line_number >= total_lines_) {
        buf[0] = '\0';
        return {};
    }

    // NOTE: For binary-search key extraction we only need a small prefix.
    // Avoid calling find_line_end_offset() here: it can trigger extra scanning/cache rebuild.
    const uint32_t start_offset = find_line_start_offset(line_number);
    if (start_offset >= file_size()) {
        buf[0] = '\0';
        return {};
    }

    const uint32_t remaining = file_size() - start_offset;
    const uint32_t to_read = std::min<uint32_t>((uint32_t)(buf_size - 1), remaining);

    if (file_.seek(start_offset) != FR_OK) {
        buf[0] = '\0';
        return {};
    }

    auto read_result = file_.read(buf, to_read);

    if (read_result.is_error() || *read_result == 0) {
        buf[0] = '\0';
        return {};
    }

    size_t n = std::min<size_t>((size_t)*read_result, buf_size - 1);

    // Trim at newline to avoid spilling into the next record.
    for (size_t i = 0; i < n; ++i) {
        if (buf[i] == '\n') {
            n = i;
            break;
        }
    }

    // Handle CRLF.
    if (n > 0 && buf[n - 1] == '\r') {
        --n;
    }

    buf[n] = '\0';
    return std::string_view{buf, n};
}

template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE> std::string FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::get_line(uint32_t line_number) {
    // Preserve the existing by-value API, but avoid additional cache-miss copies.
    // (The reference-returning overload is used internally for no-churn lookups.)
    const std::string &s = get_line_ref(line_number);
    return std::string{s};
}

template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE>
const std::string &FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::get_line_ref(uint32_t line_number) {
    static const std::string empty;

    if (line_number >= total_lines_) {
        return empty;
    }

    std::string *cached = find_cached_line(line_number);
    if (cached) {
        cache_hits_++;
        return *cached;
    }

    std::string content = read_line_from_file(line_number);

    // Remove trailing newline if present (normally not included, but keep legacy safety).
    if (!content.empty() && content.back() == '\n') {
        content.pop_back();
    }

    std::string &stored = cache_line_move(line_number, std::move(content));
    cache_misses_++;
    return stored;
}

template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE>
void FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::prefetch(uint32_t from_line, uint32_t count) {
    if (from_line >= total_lines_) {
        return;
    }

    uint32_t end_line = std::min(from_line + count, total_lines_);

    for (uint32_t line = from_line; line < end_line; ++line) {
        // Only prefetch if not already cached
        if (!find_cached_line(line)) {
            std::string content = read_line_from_file(line);
            if (!content.empty()) {
                // Keep heap peak low by moving the allocation into the cache.
                cache_line_move(line, std::move(content));
            }
        }
    }
}

template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE>
void FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::shift_file_content_right(uint32_t from_offset, uint32_t shift_amount) {
    if (shift_amount == 0) {
        return;
    }

    uint32_t file_end = file_size();
    if (from_offset >= file_end) {
        return;
    }

    uint32_t bytes_to_move = file_end - from_offset;

    // Move content in chunks from end to beginning
    while (bytes_to_move > 0) {
        uint32_t chunk_size = std::min(BUFFER_SIZE, bytes_to_move);
        uint32_t source_offset = file_end - chunk_size;

        // Read chunk from current position
        file_.seek(source_offset);
        auto result = file_.read(work_buffer_, chunk_size);

        if (result.is_error() || *result == 0) {
            break;
        }

        // Write chunk to new position
        file_.seek(source_offset + shift_amount);
        file_.write(work_buffer_, *result);

        file_end -= *result;
        bytes_to_move -= *result;
    }
}

template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE>
void FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::shift_file_content_left(uint32_t from_offset, uint32_t shift_amount) {
    if (shift_amount == 0) {
        return;
    }

    uint32_t file_end = file_size();
    if (from_offset < file_end) {

        uint32_t read_offset = from_offset;
        uint32_t write_offset = from_offset - shift_amount;

        while (read_offset < file_end) {
            uint32_t to_read = std::min(BUFFER_SIZE, file_end - read_offset);

            file_.seek(read_offset);
            auto result = file_.read(work_buffer_, to_read);

            if (result.is_error() || *result == 0) {
                break;
            }

            file_.seek(write_offset);
            file_.write(work_buffer_, *result);

            read_offset += *result;
            write_offset += *result;
        }
    }

    // Truncate file to new size
    file_.seek(file_end - shift_amount);
    file_.truncate();
}

template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE>
void FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::update_newline_cache_after_edit(uint32_t from_offset, int32_t size_delta) {
    if (size_delta == 0) {
        return;
    }

    // Update offsets in the ring buffer
    for (uint32_t i = 0; i < newline_cache_.size(); ++i) {
        NewlineEntry &entry = newline_cache_[i];

        if (entry.start_offset >= from_offset) {
            entry.start_offset += size_delta;
        }

        if (entry.end_offset >= from_offset) {
            if (size_delta < 0 && entry.end_offset < from_offset + (-size_delta)) {
                // This entry was deleted - clear cache for simplicity
                newline_cache_.clear();
                return;
            } else {
                entry.end_offset += size_delta;
            }
        }
    }
}

template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE> void FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::ensure_newline_at_eof() {
    // LOG("Ensuring newline at eof: ");
    if (file_size() == 0) {
        // LOG("empty!\n");
        return;
    }

    file_.seek(file_size() - 1);
    char last_char;
    auto result = file_.read(&last_char, 1);

    if (result.is_ok() && *result == 1 && last_char != '\n') {
        file_.seek(file_size());
        file_.write("\n", 1);
        // LOG("wrote \\n at eof\n");
    } else {
        // LOG("already \\n eof!\n");
    }
}

template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE>
void FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::append_line(const std::string &content) {
    bool add_newline = content.empty() || content.back() != '\n';
    ensure_newline_at_eof();

    file_.seek(file_size());
    file_.write(content.data(), content.length());

    if (add_newline) {
        file_.write("\n", 1);
    }

    file_.sync();

    // Update caches incrementally
    total_lines_++;

    // Cache the new line
    cache_line(total_lines_ - 1, content);

    // Add newline to cache if there's space
    if (!newline_cache_.isFull()) {
        uint32_t newline_offset = file_size() - 1;
        uint32_t line_start = newline_offset - content.length();
        if (add_newline) {
            line_start--; // Account for added newline
        }
        newline_cache_.push(NewlineEntry(total_lines_ - 1, line_start, newline_offset));
    }
}

// template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE> void FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::log() {

//     for (int i = 0; i < newline_cache_.size(); i++) {
//         int ln = newline_cache_[i].offset;
//         int dln = i > 0 ? ln - newline_cache_[i - 1].offset : -1;
//         //LOG("lc:%d:%d:%d%s\n", i, newline_cache_[i].offset, newline_cache_[i].line_number, i > 0 && dln < 2 ? " ERR!" : "");
//     }
// }

template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE>
void FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::replace_line(uint32_t line_number, const std::string &content) {
    if (line_number >= total_lines_) {
        // Line doesn't exist - append empty lines until we reach it
        while (total_lines_ <= line_number) {
            append_line("");
        }
    }

    // LOG("**** before replace\n");
    // log();

    uint32_t line_start = find_line_start_offset(line_number);
    uint32_t line_end = find_line_end_offset(line_number);
    uint32_t old_length = line_end - line_start + 1; // account for the new line

    // Prepare new content with newline
    std::string new_content = content;
    if (new_content.empty() || new_content.back() != '\n') {
        new_content += '\n';
    }
    uint32_t new_length = new_content.length();
    int32_t size_delta = new_length - old_length;

    // LOG("start:%d,end:%d,old_length:%d\n", line_start, line_end, old_length);
    // LOG("new:%d,delta:%d\n", new_length, size_delta);

    if (size_delta > 0) {
        // Need to make space
        shift_file_content_right(line_end, size_delta);
    } else if (size_delta < 0) {
        // Need to remove space
        shift_file_content_left(line_end, -size_delta);
    }

    // Write new content

    file_.seek(line_start);
    file_.write(new_content.data(), new_content.length());
    file_.sync();

    // Update caches
    invalidate_lines_from(line_number);
    cache_line(line_number, content); // Cache without newline
    update_newline_cache_after_edit(line_start, size_delta);

    // LOG("**** after replace\n");
    // log();
}

template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE>
void FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::insert_line(uint32_t line_number, const std::string &content) {
    // Ensure we have enough lines
    while (total_lines_ <= line_number) {
        append_line("");
    }

    // Replace the line
    replace_line(line_number, content);
}

template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE> void FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::delete_line(uint32_t line_number) {
    if (line_number >= total_lines_) {
        return;
    }

    uint32_t line_start = find_line_start_offset(line_number);
    uint32_t line_end = find_line_end_offset(line_number);
    uint32_t line_length = line_end - line_start + 1; // Include newline

    // Shift content left to remove the line
    shift_file_content_left(line_end + 1, line_length);

    total_lines_--;

    // Update caches
    invalidate_lines_from(line_number);
    update_newline_cache_after_edit(line_start, -(int32_t)line_length);
}
} // namespace io
#endif // FILE_WRAPPER_HPP
