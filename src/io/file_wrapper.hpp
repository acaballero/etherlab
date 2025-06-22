//
// Created by Angel Dust on 18/16/2025.
//
#pragma once

#include "dsp/decimation/dsp_fir_decimator_float.h"
#include "ff.h"
#include "io/fatfs_file.h"
#include "printf.h"
#include "ring_buffer.hpp"
#include "status.h"
#include <cstdio>
#include <string>
#include <cstring>
#include <sys/_stdint.h>

static constexpr uint32_t BUFFER_SIZE = 512;

enum FindMode { LTE, GTE, EQ };

template <uint32_t LINE_CACHE_SIZE = 12, uint32_t NEWLINE_CACHE_SIZE = 64> class FileWrapper {
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
        uint32_t line_number; // Which line this newline ends
        uint32_t offset;      // File offset of the newline character

        NewlineEntry() : line_number(0), offset(0) {
        }
        NewlineEntry(uint32_t line, uint32_t off) : line_number(line), offset(off) {
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

    bool load(io::path path, bool create) {

        file_.close();
        path_ = path;
        auto res = file_.open(path, false, create);

        if (res.ok()) {

            invalidate_all_caches();
            scan_file();

	    memcpy(void *__restrict dest, DspFIRDecimatorFloat<>ize_t n);
	    •DspFIRDecimatorFloat<int TAPS, typename T>fdf

            // LOG("**** INIT\n");
            for (int i = 0; i < newline_cache_.size(); i++) {
                // LOG("lc:%d:%d:%d\n", i, newline_cache_[i].offset, newline_cache_[i].line_number);
            }
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

    // Prefetch lines starting from a given line
    void prefetch(uint32_t from_line, uint32_t count = LINE_CACHE_SIZE);

    // Modify operations (in-place editing)
    void replace_line(uint32_t line_number, const std::string &content);
    void insert_line(uint32_t line_number, const std::string &content);
    void append_line(const std::string &content);
    void delete_line(uint32_t line_number);

    void log();

    // Generic range queries for sorted data
    template <typename T, typename ExtractKey> FRESULT find_range(const T &min_key, const T &max_key, ExtractKey extract_key, std::vector<uint32_t> &results);

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
    uint32_t find_lru_cache_slot();
    void invalidate_lines_from(uint32_t line_number);

    // File structure operations using RingBuffer
    std::string read_line_from_file(uint32_t line_number);
    uint32_t find_line_start_offset(uint32_t line_number);
    uint32_t find_line_end_offset(uint32_t line_number);
    void ensure_newline_cache_covers(uint32_t line_number);
    void update_newline_cache_after_edit(uint32_t from_offset, int32_t size_delta);

    // Low-level file manipulation
    void shift_file_content_right(uint32_t from_offset, uint32_t shift_amount);
    void shift_file_content_left(uint32_t from_offset, uint32_t shift_amount);
    void ensure_newline_at_eof();

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
    uint32_t result = total_lines_; // Not found
    T result_key;
    while (left < right) {
        uint32_t mid = left + (right - left) / 2;
        std::string line = get_line(mid);

        if (line.empty()) {
            // LOG("FR_INT_ERROR\n");
            return io::filesystem_error{FR_INT_ERR};
        }

        T line_key = extract_key(line);

        if (mode == GTE) {
            if (line_key >= key) {
                result = mid;
                result_key = key;
                right = mid;
            } else {
                left = mid + 1;
            }
        } else {
            if (line_key <= key) {
                result = mid;
                result_key = key;
                left = mid + 1;
            } else {
                right = mid;
            }
        }
    }

    if (mode == EQ && result_key != key) {
        return (uint32_t)total_lines_;
    } else {
        return (uint32_t)result;
    }
}

// Generic range search implementation
template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE>
template <typename T, typename ExtractKey>
FRESULT FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::find_range(const T &min_key, const T &max_key, ExtractKey extract_key,
                                                                     std::vector<uint32_t> &results) {

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

    if (end >= total_lines_) {
        end = total_lines_ - 1;
    }

    if (end >= start) {
        // Collect all line numbers in range
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
    total_lines_ = 1; // At least one line if file has content
    cache_start_line_ = 0;
    uint32_t offset = 0;
    uint32_t current_line = 0;

    while (offset < file_size()) {
        uint32_t to_read = std::min((uint32_t)BUFFER_SIZE, file_size() - offset);
        auto result = file_.read(work_buffer_, to_read);

        if (result.is_error() || *result == 0) {
            break;
        }

        for (uint32_t i = 0; i < *result; ++i) {
            if (work_buffer_[i] == '\n') {
                // Always count the line
                total_lines_++;

                // Only cache if there's space
                if (!newline_cache_.isFull()) {
                    newline_cache_.push(NewlineEntry(current_line, offset + i));
                }

                current_line++;
            }
        }

        offset += *result;
        if (*result < to_read) {
            break;
        }
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
void FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::invalidate_lines_from(uint32_t line_number) {
    for (uint32_t i = 0; i < LINE_CACHE_SIZE; ++i) {
        if (line_cache_[i].valid && line_cache_[i].line_number >= line_number) {
            line_cache_[i].invalidate();
        }
    }
}

template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE>
void FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::ensure_newline_cache_covers(uint32_t line_number) {
    // Check if line is already covered by cache
    if (!newline_cache_.empty()) {
        uint32_t cache_end_line = cache_start_line_ + newline_cache_.size();
        if (line_number >= cache_start_line_ && line_number < cache_end_line) {
            return; // Already covered
        }
    }

    // Need to rebuild cache around this line
    newline_cache_.clear();

    // Start scanning from beginning of file to find the target line
    file_.seek(0);
    uint32_t offset = 0;
    uint32_t current_line = 0;
    cache_start_line_ = 0;

    // Scan until we reach the target line or fill the cache
    while (offset < file_size() && !newline_cache_.isFull()) {
        uint32_t to_read = std::min(BUFFER_SIZE, file_size() - offset);
        auto result = file_.read(work_buffer_, to_read);

        if (result.is_error() || *result == 0) {
            break;
        }

        for (uint32_t i = 0; i < *result; ++i) {
            if (work_buffer_[i] == '\n') {
                newline_cache_.push(NewlineEntry(current_line, offset + i));
                current_line++;

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
uint32_t FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::find_line_start_offset(uint32_t line_number) {
    if (line_number == 0) {
        return 0;
    }

    ensure_newline_cache_covers(line_number - 1);

    // Look for the newline that ends the previous line
    uint32_t prev_line = line_number - 1;
    for (uint32_t i = 0; i < newline_cache_.size(); ++i) {
        const NewlineEntry &entry = newline_cache_[i];
        if (entry.line_number == prev_line) {
            return entry.offset + 1;
        }
    }

    // Fallback: scan from beginning
    file_.seek(0);
    uint32_t offset = 0;
    uint32_t current_line = 0;

    while (offset < file_size() && current_line < line_number) {
        uint32_t to_read = std::min(BUFFER_SIZE, file_size() - offset);
        auto result = file_.read(work_buffer_, to_read);

        if (result.is_error() || *result == 0) {
            break;
        }

        for (uint32_t i = 0; i < *result; ++i) {
            if (work_buffer_[i] == '\n') {
                current_line++;
                if (current_line == line_number) {
                    return offset + i + 1;
                }
            }
        }

        offset += *result;
        if (*result < to_read) {
            break;
        }
    }

    return offset;
}

template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE>
uint32_t FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::find_line_end_offset(uint32_t line_number) {
    ensure_newline_cache_covers(line_number);

    // Look for the newline that ends this line
    for (uint32_t i = 0; i < newline_cache_.size(); ++i) {
        const NewlineEntry &entry = newline_cache_[i];
        if (entry.line_number == line_number) {
            return entry.offset;
        }
    }

    // Fallback: scan from line start
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

    return result;
}

template <uint32_t LINE_CACHE_SIZE, uint32_t NEWLINE_CACHE_SIZE> std::string FileWrapper<LINE_CACHE_SIZE, NEWLINE_CACHE_SIZE>::get_line(uint32_t line_number) {
    if (line_number >= total_lines_) {
        return {};
    }

    std::string content;
    std::string *cached = find_cached_line(line_number);
    if (cached) {
        content = *cached;
        cache_hits_++;
    } else {

        content = read_line_from_file(line_number);

        cache_line(line_number, content);

        cache_misses_++;
    }

    // Remove trailing newline if present
    if (!content.empty() && content.back() == '\n') {
        content.pop_back();
    }

    return content;
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
                cache_line(line, content);
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

        if (entry.offset >= from_offset) {
            if (size_delta < 0 && entry.offset < from_offset + (-size_delta)) {
                // This entry was deleted - remove it by marking invalid
                // We'll handle removal by rebuilding cache lazily
                continue;
            } else {
                // Adjust offset
                entry.offset += size_delta;
            }
        }
    }

    // For simplicity, clear cache if we had deletions
    // More sophisticated approach would compact the ring buffer
    if (size_delta < 0) {
        newline_cache_.clear();
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

    // LOG("Append line\n");
    ensure_newline_at_eof();

    file_.seek(file_size());
    file_.write(content.data(), content.length());

    if (content.empty() || content.back() != '\n') {
        file_.write("\n", 1);
    }

    file_.sync();

    // Update caches incrementally
    total_lines_++;

    // Cache the new line
    cache_line(total_lines_ - 1, content);

    // Add newline to cache if there's space
    if (!newline_cache_.isFull()) {
        // LOG("File size is %d, writing newline at %d\n", file_size(), file_size() - 1);
        uint32_t newline_offset = file_size() - 1;
        newline_cache_.push(NewlineEntry(total_lines_ - 1, newline_offset));
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
