// ============================================================================
// Prevent STM32 includes and provide mocks FIRST
// ============================================================================
#ifndef LINUX_BUILD
#define LINUX_BUILD

#include <ostream>
#endif

#define TRX_FRONTEND_STM32_H

#define STM32F4XX_H     // Prevent stm32f4xx.h inclusion
#define STM32F4XX_HAL_H // Prevent HAL header inclusion
#define _ARM_MATH_H     // Prevent ARM math header inclusion

#include <stdint.h>
#include <cmath>
#include <algorithm>

// Mock STM32/ARM functions
extern "C" {
uint32_t HAL_GetTick() {
    return 0;
}

int32_t __SSAT(int32_t val, int32_t sat) {
    int32_t max = (1 << (sat - 1)) - 1;
    int32_t min = -(1 << (sat - 1));
    return (val > max) ? max : (val < min) ? min : val;
}
}

// Mock ARM SIMD intrinsics
#define __SIMD32(addr) (*(int32_t **)&(addr))
#define __SMUAD(x, y) ((int32_t)(((short)(x) * (short)(y)) + (((short)((x) >> 16)) * ((short)((y) >> 16)))))
#define __PKHBT(a, b, shift) ((uint32_t)(((uint32_t)(a)&0xFFFFU) | (((uint32_t)(b)&0xFFFFU) << (shift))))
#define __PKHTB(a, b, shift) ((uint32_t)(((uint32_t)(a)&0xFFFF0000U) | (((uint32_t)(b) >> (shift)) & 0xFFFFU)))

// Now include standard headers
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <cstring>
#include <iomanip>
#include "wavreader.h"
#define LOG printf
#define printf_ printf

// Auto-mock missing functions/namespaces
namespace sstrength {
struct Signal {
    void *add(void *, void *) {
        return nullptr;
    }
    void remove(void *) {
    }
};
Signal squelch_signal;
} // namespace sstrength
namespace dsp {
bool apply_audio_bpf() {
    return true;
}
template <typename T> T max2(T a, T b) {
    return std::max(a, b);
}
} // namespace dsp
enum BeepType { BEEP_SUCCESS = 0 };

// ============================================================================
// Include Your Actual APRS Code
// ============================================================================

#include "aprs_packet.h"
#include "aprs_decoder.h"

using namespace dsp;

// ============================================================================
// Benchmark Statistics
// ============================================================================

struct BenchmarkStats {
    int total_packets = 0, valid_packets = 0;
    std::chrono::milliseconds processing_time{0};
    float audio_duration = 0.0f;
    float scale = 0.0f;
    float alpha = 0.0f;
    float th = 0.0f;
    std::vector<std::string> packet_info;

    void print_summary() {
        std::cout << "\n=== APRS Benchmark Results ===" << std::endl;
        std::cout << "Audio: " << audio_duration << "s, Processing: " << processing_time.count() << "ms ";
        std::cout << "Real-time factor: " << (audio_duration * 1000.0f) / processing_time.count() << "x" << std::endl;
        std::cout << "Packets: " << total_packets << " total, " << valid_packets << " valid";
        if (total_packets > 0)
            std::cout << " (" << (100.0f * valid_packets / total_packets) << "%)";
        std::cout << std::endl;
        //  for (const auto &info : packet_info)
        //    std::cout << "  " << info << std::endl;
    }
} g_stats;

// ============================================================================
// Main
// ============================================================================

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <wav_file>" << std::endl;
        return 1;
    }

    WAVReader reader;
    if (!reader.load(argv[1])) {
        std::cerr << "Error loading WAV file" << std::endl;
        return 1;
    }

    // Your actual APRS decoder
    APRSTask decoder;
    decoder.sample_rate = reader.sample_rate;

    decoder.on_packet = [](APRSPacket *pkt) {
        g_stats.total_packets++;
        if (pkt->is_valid_checksum()) {
            g_stats.valid_packets++;
        }

        // char src[15], dst[15];
        // pkt->get_source_formatted(src);
        // pkt->get_destination_formatted(dst);
        // std::string info = pkt->get_information_text_formatted();

        // g_stats.packet_info.push_back(std::string(src) + " -> " + std::string(dst) + (info.empty() ? "" : " | " + info.substr(0, 30)));

        // std::cout << "Packet #" << g_stats.total_packets <<  ": " << g_stats.packet_info.back() << std::endl;
    };

    BenchmarkStats best{};
    float alpha = 0;
    int first_packet_end_ix = -1;

    for (float scale = 20; scale <= 80; scale += 10) {
        for (float alpha = 0.01; alpha <= 0.2; alpha += 0.01f) {
            for (float th = 0.5; th <= 0.9; th += 0.1f) {

                g_stats = {};
                g_stats.audio_duration = (float)reader.samples.size() / reader.sample_rate;
                g_stats.scale = scale;
                g_stats.alpha = alpha;
                g_stats.th = th;
                decoder.scale = scale;
                decoder.alpha = alpha;
                decoder.noise_threshold = th;
                decoder.bit_threshold = scale * 4;
                decoder.phase_adjust = 1.0f / 32;
                // decoder.bit_threshold = th * scale;

                if (!decoder.init()) {
                    std::cerr << "Error initializing APRS decoder" << std::endl;
                    return 1;
                }

                std::cout << "Processing " << reader.samples.size() << " samples: scale: " << decoder.scale << " | alpha: " << decoder.alpha
                          << " | th: " << decoder.noise_threshold << " | bit th: " << decoder.bit_threshold << std::endl;

                auto start = std::chrono::steady_clock::now();

                // Process in chunks
                const size_t chunk_size = 512;
                first_packet_end_ix = -1;
                for (size_t i = 0; i < reader.samples.size(); i += chunk_size) {
                    size_t len = std::min(chunk_size, reader.samples.size() - i);

                    buffer_t<float_t> buffer(&reader.samples[i], len);
                    int pkt_end_ix = decoder.process_audio(buffer);

                    if (pkt_end_ix >= 0) {
                        pkt_end_ix += i;
                        if (first_packet_end_ix < 0) {
                            first_packet_end_ix = pkt_end_ix;
                        }
                        //     std::cout << "Packet end at " << pkt_end_ix / reader.sample_rate << std::endl;
                    }

                    if (i % (reader.samples.size() / 10) == 0) {
                        std::cout << "\rProgress: " << (100 * i / reader.samples.size()) << "%" << std::flush;
                    }
                }

                std::cout << "\rFirst packet found at " << first_packet_end_ix / reader.sample_rate << std::endl;

                auto end = std::chrono::steady_clock::now();
                g_stats.processing_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

                std::cout << "\rProgress: 100%" << std::endl;

                if (g_stats.valid_packets > best.valid_packets) {
                    best = g_stats;
                }
                g_stats.print_summary();
            }
        }
    }

    std::cout << "\rBest configuration: packets: " << best.valid_packets << " | alpha: " << best.alpha << " | threshold: " << best.th
              << " | scale: " << best.scale << std::endl;

    return 0;
}
