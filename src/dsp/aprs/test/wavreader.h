#define TRX_FRONTEND_STM32_H

#define STM32F4XX_H     // Prevent stm32f4xx.h inclusion
#define STM32F4XX_HAL_H // Prevent HAL header inclusion
#define _ARM_MATH_H     // Prevent ARM math header inclusion

#include <cmath>
#include <stdint.h>
#include <algorithm>
#include <vector>
#include <stdio.h>
#include <string>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <fstream>

class WAVReader {
  public:
    std::vector<float> samples;
    uint32_t sample_rate;

    bool load(const std::string &filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Cannot open file: " << filename << std::endl;
            return false;
        }

        // Read RIFF header
        char riff[4];
        file.read(riff, 4);
        if (strncmp(riff, "RIFF", 4) != 0) {
            std::cerr << "Not a RIFF file" << std::endl;
            return false;
        }

        uint32_t file_size;
        file.read(reinterpret_cast<char *>(&file_size), 4);

        char wave[4];
        file.read(wave, 4);
        if (strncmp(wave, "WAVE", 4) != 0) {
            std::cerr << "Not a WAVE file" << std::endl;
            return false;
        }

        // Find fmt chunk
        uint16_t channels = 0, bits_per_sample = 0;
        bool found_fmt = false, found_data = false;
        uint32_t data_size = 0;

        while (!file.eof() && (!found_fmt || !found_data)) {
            char chunk_id[4];
            uint32_t chunk_size;

            file.read(chunk_id, 4);
            file.read(reinterpret_cast<char *>(&chunk_size), 4);

            if (strncmp(chunk_id, "fmt ", 4) == 0) {
                uint16_t audio_format;
                file.read(reinterpret_cast<char *>(&audio_format), 2);
                file.read(reinterpret_cast<char *>(&channels), 2);
                file.read(reinterpret_cast<char *>(&sample_rate), 4);

                uint32_t byte_rate;
                uint16_t block_align;
                file.read(reinterpret_cast<char *>(&byte_rate), 4);
                file.read(reinterpret_cast<char *>(&block_align), 2);
                file.read(reinterpret_cast<char *>(&bits_per_sample), 2);

                // Skip any extra fmt data
                if (chunk_size > 16) {
                    file.seekg(chunk_size - 16, std::ios::cur);
                }

                if (audio_format != 1 && audio_format != 3) {
                    std::cerr << "Unsupported audio format: " << audio_format << std::endl;
                    return false;
                }
                found_fmt = true;

            } else if (strncmp(chunk_id, "data", 4) == 0) {
                data_size = chunk_size;
                found_data = true;
                break;

            } else {
                // Skip unknown chunk
                file.seekg(chunk_size, std::ios::cur);
            }
        }

        if (!found_fmt || !found_data) {
            std::cerr << "Missing fmt or data chunk" << std::endl;
            return false;
        }

        std::cout << "WAV: " << sample_rate << "Hz, " << channels << " ch, " << bits_per_sample << " bits, "
                  << (float)data_size / (sample_rate * channels * (bits_per_sample / 8)) << "s" << std::endl;

        // Read audio data
        if (bits_per_sample == 16) {
            std::vector<int16_t> raw(data_size / 2);
            file.read(reinterpret_cast<char *>(raw.data()), data_size);

            samples.reserve(raw.size() / channels);
            for (size_t i = 0; i < raw.size(); i += channels) {
                float sample = raw[i] / 32768.0f;
                if (channels == 2 && i + 1 < raw.size()) {
                    sample = (sample + raw[i + 1] / 32768.0f) / 2.0f;
                }
                samples.push_back(sample);
            }

        } else if (bits_per_sample == 32) {
            std::vector<float> raw(data_size / 4);
            file.read(reinterpret_cast<char *>(raw.data()), data_size);

            samples.reserve(raw.size() / channels);
            for (size_t i = 0; i < raw.size(); i += channels) {
                float sample = raw[i];
                if (channels == 2 && i + 1 < raw.size()) {
                    sample = (sample + raw[i + 1]) / 2.0f;
                }
                samples.push_back(sample);
            }

        } else {
            std::cerr << "Unsupported bit depth: " << bits_per_sample << std::endl;
            return false;
        }

        return true;
    }
};
