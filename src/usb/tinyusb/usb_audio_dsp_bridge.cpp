#include "usb_audio_dsp_bridge.h"
#include "dsp/dsp_common.h"
#include "status.h"
#include "tusb.h"
#include <string.h>

// Ring buffers for audio
#define RX_RING_BUFFER_SIZE (USB_AUDIO_BUFFER_SAMPLES * 2) // 16ms buffer
#define TX_RING_BUFFER_SIZE (USB_AUDIO_BUFFER_SAMPLES * 2)

static int16_t rx_ring_buffer[RX_RING_BUFFER_SIZE];
static volatile uint16_t rx_write_pos = 0;
static volatile uint16_t rx_read_pos = 0;

static int16_t tx_ring_buffer[TX_RING_BUFFER_SIZE];
static volatile uint16_t tx_write_pos = 0;
static volatile uint16_t tx_read_pos = 0;

static bool is_streaming = false;

// Sample rate converter state (if needed)
static uint32_t src_accumulator = 0;
static int16_t last_sample = 0;

void usb_audio_dsp_bridge_init(void) {
    memset(rx_ring_buffer, 0, sizeof(rx_ring_buffer));
    memset(tx_ring_buffer, 0, sizeof(tx_ring_buffer));
    rx_write_pos = 0;
    rx_read_pos = 0;
    tx_write_pos = 0;
    tx_read_pos = 0;
    is_streaming = false;
    src_accumulator = 0;
    last_sample = 0;
}

void usb_audio_dsp_bridge_start(void) {
    is_streaming = true;
}

void usb_audio_dsp_bridge_stop(void) {
    is_streaming = false;
}

bool usb_audio_is_streaming(void) {
    return is_streaming && tud_mounted();
}

// Convert float32 audio to int16 and write to RX ring buffer
void usb_audio_send_rx_audio(const int16_t *audio_samples, uint16_t count) {
    if (!is_streaming || !audio_samples) {
        return;
    }

    for (uint16_t i = 0; i < count; i++) {

        // Write to ring buffer
        rx_ring_buffer[rx_write_pos] = audio_samples[i];
        rx_write_pos = (rx_write_pos + 1) % RX_RING_BUFFER_SIZE;

        // Check for overflow
        if (rx_write_pos == rx_read_pos) {
            // Buffer overflow - advance read position
            rx_read_pos = (rx_read_pos + 1) % RX_RING_BUFFER_SIZE;
        }
    }
}

// Read int16 audio from TX ring buffer and convert to float32
uint16_t usb_audio_get_tx_audio(float32_t *audio_samples, uint16_t max_count) {
    if (!is_streaming || !audio_samples) {
        memset(audio_samples, 0, max_count * sizeof(float32_t));
        return 0;
    }

    uint16_t available = 0;
    if (tx_write_pos >= tx_read_pos) {
        available = tx_write_pos - tx_read_pos;
    } else {
        available = TX_RING_BUFFER_SIZE - tx_read_pos + tx_write_pos;
    }

    uint16_t to_read = available < max_count ? available : max_count;

    for (uint16_t i = 0; i < to_read; i++) {
        int16_t sample_i16 = tx_ring_buffer[tx_read_pos];
        tx_read_pos = (tx_read_pos + 1) % TX_RING_BUFFER_SIZE;

        // Convert to float32
        audio_samples[i] = (float32_t)sample_i16 / 32768.0f;
    }

    // Fill remaining with zeros if not enough samples
    if (to_read < max_count) {
        memset(&audio_samples[to_read], 0, (max_count - to_read) * sizeof(float32_t));
    }

    return to_read;
}

void usb_audio_process(void) {

    if (!tud_mounted() || !is_streaming) {
        return;
    }

    // Send RX audio to USB (PC <- Radio)
    uint16_t available = 0;
    if (rx_write_pos >= rx_read_pos) {
        available = rx_write_pos - rx_read_pos;
    } else {
        available = RX_RING_BUFFER_SIZE - rx_read_pos + rx_write_pos;
    }

    if (available >= USB_AUDIO_BUFFER_SAMPLES) {
        int16_t usb_buffer[USB_AUDIO_BUFFER_SAMPLES];

        for (uint16_t i = 0; i < USB_AUDIO_BUFFER_SAMPLES; i++) {
            usb_buffer[i] = rx_ring_buffer[rx_read_pos];
            rx_read_pos = (rx_read_pos + 1) % RX_RING_BUFFER_SIZE;
        }

        // Send to USB
        tud_audio_write((uint8_t *)usb_buffer, USB_AUDIO_BUFFER_SIZE);
    }

#if CFG_TUD_AUDIO_ENABLE_EP_OUT
    // Receive TX audio from USB (PC -> Radio)
    if (tud_audio_available()) {
        int16_t usb_buffer[USB_AUDIO_BUFFER_SAMPLES];
        uint16_t bytes_read = tud_audio_read((uint8_t *)usb_buffer, USB_AUDIO_BUFFER_SIZE);
        uint16_t samples_read = bytes_read / sizeof(int16_t);

        for (uint16_t i = 0; i < samples_read; i++) {
            tx_ring_buffer[tx_write_pos] = usb_buffer[i];
            tx_write_pos = (tx_write_pos + 1) % TX_RING_BUFFER_SIZE;

            // Check for overflow
            if (tx_write_pos == tx_read_pos) {
                // Buffer overflow - advance read position
                tx_read_pos = (tx_read_pos + 1) % TX_RING_BUFFER_SIZE;
            }
        }
    }
#endif
}

// TinyUSB callbacks
void tud_audio_rx_done_cb(uint8_t rhport, uint16_t n_bytes_received, uint8_t func_id, uint8_t ep_out, uint8_t cur_alt_setting) {
    (void)rhport;
    (void)n_bytes_received;
    (void)func_id;
    (void)ep_out;

    if (cur_alt_setting != 0) {
        is_streaming = true;
    } else {
        is_streaming = false;
    }
}

void tud_audio_tx_done_cb(uint8_t rhport, uint16_t n_bytes_sent, uint8_t func_id, uint8_t ep_in, uint8_t cur_alt_setting) {
    (void)rhport;
    (void)n_bytes_sent;
    (void)func_id;
    (void)ep_in;

    if (cur_alt_setting != 0) {
        is_streaming = true;
    } else {
        is_streaming = false;
    }
}
