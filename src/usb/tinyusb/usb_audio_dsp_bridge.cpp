#include "usb_audio_dsp_bridge.h"
#include "common/tusb_verify.h"
#include "dsp/decimation/dsp_fir_decimator_float.h"
#include "dsp/decimation/dsp_fir_decimator_q15.h"
#include "dsp/decimation/dsp_iir_decimator.h"
#include "dsp/dsp_buffers.h"
#include "dsp/dsp_common.h"
#include "dsp/fir_filter.h"
#include "memory_allocator.h"
#include "status.h"
#include "tinyusb/tusb_config.h"
#include "tinyusb/usb_composite_device.h"
#include "tusb.h"
#include <string.h>

// Ring buffer for audio
#define RING_BUFFER_SIZE (USB_AUDIO_BUFFER_SAMPLES * 3)
//#define TX_RING_BUFFER_SIZE (USB_AUDIO_BUFFER_SAMPLES * 2)

static int16_t *ring_buffer;
static volatile uint16_t write_pos = 0;
static volatile uint16_t read_pos = 0;

// static int16_t tx_ring_buffer[TX_RING_BUFFER_SIZE];
// static volatile uint16_t tx_write_pos = 0;
// static volatile uint16_t tx_read_pos = 0;

static bool is_streaming[2] = {false, false};
// static bool is_streaming = false;

// FIR instance and state buffer

DspFIRDecimatorFloat<FIR_DECIMATOR_1ST_HALFBAND_TAPS> decimator;
uint16_t filter_rate = USB_AUDIO_SAMPLE_RATE;
uint16_t filter_bandwidth = 3000;

// Initialize FIR filter
void init_filter(uint16_t output_rate) {
    uint16_t bw = USB_AUDIO_SAMPLE_RATE / 2;
    while (bw > output_rate / 2) {
        bw /= 2;
    }

    bool b = decimator.config(USB_AUDIO_SAMPLE_RATE, bw, 1);
    if (b) {
        filter_rate = output_rate;
        return;
    }
    LOG("usb audio bridge: Error initializing audio filter\n");
    TU_BREAKPOINT();
}

void usb_audio_dsp_bridge_init(void) {

    ring_buffer = (int16_t *)CCMMemoryAllocator::alloc(RING_BUFFER_SIZE * sizeof(int16_t));
    memset(ring_buffer, 0, RING_BUFFER_SIZE * sizeof(int16_t));
    // memset(tx_ring_buffer, 0, sizeof(tx_ring_bufyfer));
    write_pos = 0;
    read_pos = 0;
    // tx_write_pos = 0;
    // tx_read_pos = 0;
    is_streaming[ITF_IX_MICROPHONE] = false;
    is_streaming[ITF_IX_SPEAKER] = false;
}

void usb_audio_dsp_bridge_start(uint8_t itf_index) {
    is_streaming[itf_index] = true;
}

void usb_audio_dsp_bridge_stop(uint8_t itf_index) {
    is_streaming[itf_index] = false;
}

bool usb_audio_is_streaming(uint8_t itf_index) {
    return is_streaming[itf_index] && usb_connected();
}

// // Convert float32 audio to int16 and write to RX ring buffer
// void usb_audio_send_rx_audio(const int16_t *audio_samples, uint16_t count) {
//     if (!is_streaming[ITF_IX_MICROPHONE] || !audio_samples) {
//         return;
//     }

//     for (uint16_t i = 0; i < count; i++) {

//         // Write to ring buffer
//         ring_buffer[write_pos] = audio_samples[i];
//         write_pos = (write_pos + 1) % RING_BUFFER_SIZE;

//         // Check for overflow
//         if (write_pos == read_pos) {
//             // Buffer overflow - advance read position
//             read_pos = (read_pos + 1) % RING_BUFFER_SIZE;
//         }
//     }
// }

// // Read int16 audio from TX ring buffer and convert to float32
// uint16_t usb_audio_get_tx_audio(float32_t *audio_samples, uint16_t max_count) {
//     if (!is_streaming[ITF_IX_SPEAKER] || !audio_samples) {
//         memset(audio_samples, 0, max_count * sizeof(float32_t));
//         return 0;
//     }

//     uint16_t available = 0;
//     if (write_pos >= read_pos) {
//         available = write_pos - read_pos;
//     } else {
//         available = RING_BUFFER_SIZE - read_pos + write_pos;
//     }

//     uint16_t to_read = available < max_count ? available : max_count;

//     for (uint16_t i = 0; i < to_read; i++) {
//         int16_t sample_i16 = ring_buffer[read_pos];
//         read_pos = (read_pos + 1) % RING_BUFFER_SIZE;

//         // Convert to float32
//         audio_samples[i] = (float32_t)sample_i16 / 32768.0f;
//     }

//     // Fill remaining with zeros if not enough samples
//     if (to_read < max_count) {
//         memset(&audio_samples[to_read], 0, (max_count - to_read) * sizeof(float32_t));
//     }

//     return to_read;
// }

void resample_linear(int16_t *input, int16_t *output, uint32_t input_count, uint32_t output_count) {

    float ratio = (float)input_count / (float)output_count;
    float phase = 0.0f;

    for (uint32_t i = 0; i < output_count; i++) {
        uint32_t index = (uint32_t)phase;
        float frac = phase - (float)index;

        // Prevent overrun
        if (index >= input_count - 1) {
            output[i] = input[input_count - 1];
            continue;
        }

        q15_t y0 = input[index];
        q15_t y1 = input[index + 1];

        int32_t diff = (int32_t)(y1 - y0) * (int32_t)(frac * 32768);
        output[i] = y0 + (q15_t)(diff >> 15);

        phase += ratio;
    }
}

float32_t tmp_buff[DSP_BLOCK];

uint32_t decimate_samples(int16_t *input, uint32_t num_input, uint32_t rate_in, int16_t *output, uint32_t rate_out) {
    uint32_t num_output = (num_input * rate_out) / rate_in;
    uint32_t ratio = (rate_in << 16) / rate_out; // Fixed point 16.16

    dsp::s16_to_f32(input, tmp_buff, num_input);

    buffer_t<float32_t> buffer = {tmp_buff, (size_t)num_input, rate_in, REAL};

    if (filter_rate != rate_out) {
        init_filter(rate_out);
    }
    //    decimator.decimate(buffer, buffer); // This decimator has factor 1: Does not decimate (cmsis filters only work for factors of 2 sample rate
    // ratios
    for (uint32_t i = 0; i < num_output; i++) {
        uint32_t src_pos = (i * ratio) >> 16;
        output[i] = tmp_buff[src_pos]; // Simple nearest-neighbor
    }

    return num_output;
}

// Receive TX audio from USB (host -> device)
// Call this from the DAC ISR handler
uint16_t usb_audio_receive(int16_t *buffer, uint32_t count, uint16_t sample_rate) {

    (void)sample_rate;

    // Note the usage of  tud_audio_n_available/tud_audio_n_read to specify the speaker interface
    // The functions without _n_ default to interface 0
    if (!usb_connected() || !is_streaming[ITF_IX_SPEAKER] || !tud_audio_n_available(ITF_IX_SPEAKER)) {
        return 0;
    }

    uint16_t input_count = (((float)USB_AUDIO_SAMPLE_RATE / sample_rate) * count) + 1;

    uint16_t bytes_read = tud_audio_n_read(ITF_IX_SPEAKER, (uint8_t *)ring_buffer, input_count * sizeof(uint16_t));
    uint16_t samples_read = bytes_read / sizeof(int16_t);

    uint16_t output_count = 0;
    if (samples_read) {

        output_count = decimate_samples(ring_buffer, samples_read, USB_AUDIO_SAMPLE_RATE, buffer, sample_rate);

        for (size_t i = 0; i < output_count; i++) {
            buffer[i] = (adc_type)(buffer[i] * usb::volume_factor[ITF_IX_SPEAKER][1]) >> 3; // 16 to 12 bit resolution
        }
    }
    // TODO: decimate/interpolate

    return output_count;
}

void usb_audio_send(int16_t *buffer, uint32_t count, uint16_t sample_rate) {

    if (!usb_connected() || !is_streaming[ITF_IX_MICROPHONE]) {
        return;
    }

    uint16_t output_count = (((float)USB_AUDIO_SAMPLE_RATE / sample_rate) * count) + 1;

    if (output_count > RING_BUFFER_SIZE) {
        status::pop_alert(status::ERROR, "usb_audio_process: Error: interpolation_factor too high for the ring size");
    }

    // Apply volume and change resolution
    for (size_t i = 0; i < count; i++) {
        buffer[i] = (adc_type)(buffer[i] * usb::volume_factor[ITF_IX_MICROPHONE][1]) << 3; // 12 to 16 bit resolution
    }

    // Linear interpolation.
    // Note we use the ring_buffer as a simple one-shot buffer. NOT as a ring buffer

    resample_linear(buffer, ring_buffer, count, output_count);

    tud_audio_write((uint8_t *)ring_buffer, output_count * sizeof(uint16_t));
}

// void usb_audio_process(void) {

//     if (!usb_connected() || !is_streaming) {
//         return;
//     }

//     // Send RX audio to USB (PC <- Radio)
//     uint16_t available = 0;
//     if (rx_write_pos >= rx_read_pos) {
//         available = rx_write_pos - rx_read_pos;
//     } else {
//         available = RX_RING_BUFFER_SIZE - rx_read_pos + rx_write_pos;
//     }

//     if (available >= USB_AUDIO_BUFFER_SAMPLES) {
//         int16_t usb_buffer[USB_AUDIO_BUFFER_SAMPLES];

//         for (uint16_t i = 0; i < USB_AUDIO_BUFFER_SAMPLES; i++) {
//             usb_buffer[i] = rx_ring_buffer[rx_read_pos];
//             rx_read_pos = (rx_read_pos + 1) % RX_RING_BUFFER_SIZE;
//         }

//         // Send to USB
//         tud_audio_write((uint8_t *)usb_buffer, USB_AUDIO_BUFFER_SIZE);
//     }

// #if CFG_TUD_AUDIO_ENABLE_EP_OUT
//     // Receive TX audio from USB (PC -> Radio)
//     if (tud_audio_available()) {
//         int16_t usb_buffer[USB_AUDIO_BUFFER_SAMPLES];
//         uint16_t bytes_read = tud_audio_read((uint8_t *)usb_buffer, USB_AUDIO_BUFFER_SIZE);
//         uint16_t samples_read = bytes_read / sizeof(int16_t);

//         for (uint16_t i = 0; i < samples_read; i++) {
//             tx_ring_buffer[tx_write_pos] = usb_buffer[i];
//             tx_write_pos = (tx_write_pos + 1) % TX_RING_BUFFER_SIZE;

//             // Check for overflow
//             if (tx_write_pos == tx_read_pos) {
//                 // Buffer overflow - advance read position
//                 tx_read_pos = (tx_read_pos + 1) % TX_RING_BUFFER_SIZE;
//             }
//         }
//     }
// #endif
// }
