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

void usb_audio_dsp_bridge_set_callback_in() {
}

void usb_audio_dsp_bridge_start(uint8_t itf_index) {
    is_streaming[itf_index] = true;
}

void usb_audio_dsp_bridge_stop(uint8_t itf_index) {
    is_streaming[itf_index] = false;
}

bool usb_audio_is_streaming(uint8_t itf_index, bool check_connection) {
    bool b = is_streaming[itf_index];
    if (b && check_connection) {
        b = usb_connected();
        if (!b) {
            usb_audio_dsp_bridge_stop(itf_index);
        }
    }

    return b;
}

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

// Receive TX audio from USB (host -> device)
uint16_t usb_audio_receive(int16_t *buffer, uint32_t count) {

    // Note the usage of  tud_audio_n_available/tud_audio_n_read to specify the speaker interface
    // The functions without _n_ default to interface 0
    if (!usb_connected() || !is_streaming[ITF_IX_SPEAKER] || !tud_audio_n_available(ITF_IX_SPEAKER)) {
        return 0;
    }

    uint16_t bytes_read = tud_audio_n_read(ITF_IX_SPEAKER, (uint8_t *)buffer, count * sizeof(uint16_t));
    uint16_t samples_read = bytes_read / sizeof(int16_t);

    for (size_t i = 0; i < samples_read; i++) {
        buffer[i] = (adc_type)(buffer[i] * usb::volume_factor[ITF_IX_SPEAKER][1]);
    }

    return samples_read;
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
    // Note: we use the ring_buffer as a simple one-shot buffer. NOT as a ring buffer
    // Note: Does not filter after interpolating to prevent images, but if the signal bandwidth is well below
    // the nyquist frequency (as it is supposed being lo-fi audio) the images may be tolerable

    resample_linear(buffer, ring_buffer, count, output_count);

    tud_audio_write((uint8_t *)ring_buffer, output_count * sizeof(uint16_t));
}
