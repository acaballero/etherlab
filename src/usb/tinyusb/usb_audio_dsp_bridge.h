#ifndef USB_AUDIO_DSP_BRIDGE_H
#define USB_AUDIO_DSP_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
typedef float float32_t;

/**
 * USB Audio DSP Bridge
 *
 * This integrates USB Audio with your existing DSP pipeline:
 *
 * RX Mode (Radio -> PC):
 *   ADC (I/Q IF) -> Decimators -> Demodulator -> process_audio() -> DAC (speaker)
 *                                                                  -> USB Audio (WSJT-X)
 *
 * TX Mode (PC -> Radio):
 *   USB Audio (WSJT-X) -> SSB Modulator -> Interpolators -> DAC (I/Q IF)
 */

// Audio format for WSJT-X
#define USB_AUDIO_SAMPLE_RATE 48000
#define USB_AUDIO_CHANNELS 1 // Mono audio for WSJT-X
#define USB_AUDIO_BIT_DEPTH 16

// Buffer for USB audio samples
#define USB_AUDIO_BUFFER_SAMPLES 192 // 4ms at 48kHz
#define USB_AUDIO_BUFFER_SIZE (USB_AUDIO_BUFFER_SAMPLES * sizeof(int16_t))

// Initialize USB audio bridge
void usb_audio_dsp_bridge_init(void);

// Start/stop USB audio streaming
void usb_audio_dsp_bridge_start(void);
void usb_audio_dsp_bridge_stop(void);

// Called from your process_audio() to send demodulated audio to USB
// This should be called AFTER your process_audio() has done filtering/compression
void usb_audio_send_rx_audio(const float32_t *audio_samples, uint16_t count);

// Called from your TX modulator to get audio from USB
// Returns number of samples written to audio_samples
uint16_t usb_audio_get_tx_audio(float32_t *audio_samples, uint16_t max_count);

// Check if USB audio is active
bool usb_audio_is_streaming(void);

// Process USB tasks (call from main loop)
void usb_audio_process(void);

#ifdef __cplusplus
}
#endif

#endif // USB_AUDIO_DSP_BRIDGE_H
