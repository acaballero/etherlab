/**
 * usb_composite_device.c
 *
 * Complete TinyUSB composite device implementation
 * Replaces all HAL USB code with TinyUSB for CDC+MSC+Audio
 */

#include "../../../lib/tinyusb/src/tusb.h"
#include "common/tusb_compiler.h"
#include "common/tusb_verify.h"
#include "device/usbd.h"
#include "dsp/dsp_buffers.h"
#include "hw/stm32f4xx/usb.h"
#include "os/task_manager.h"
#include "status.h"
#include "stm32f4xx_hal_tim.h"
#include "tinyusb/tusb_config.h"
#include "usb_composite_device.h"
#include "usb_audio_dsp_bridge.h"
#include "io/cat_if.h"
#include "hw/stm32_hal.h"
#include "version.h"
#include <stm32f427xx.h>
#include <string.h>
#include <sys/_stdint.h>

// SD card functions (you'll need to provide these)
extern int sd_card_read_blocks(uint32_t lba, uint8_t *buffer, uint32_t block_count);
extern int sd_card_write_blocks(uint32_t lba, const uint8_t *buffer, uint32_t block_count);
extern uint32_t sd_card_get_block_count(void);
extern bool sd_card_is_ready(void);

// USB device state
static bool cdc_connected = false;
static bool msc_connected = false;

namespace usb {

bool mute[CFG_TUD_AUDIO][CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1];           // +1 for master channel 0
int16_t volume_db[CFG_TUD_AUDIO][CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1];   // +1 for master channel 0
float volume_factor[CFG_TUD_AUDIO][CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1]; // +1 for master channel 0

} // namespace usb
//--------------------------------------------------------------------+
// INITIALIZATION
//--------------------------------------------------------------------+

uint32_t sampFreq;
uint8_t clkValid;
int16_t vol_max_db = 12;
int16_t vol_min_db = -50;
int16_t host_max_db = 90;
int16_t host_min_db = -90;

// Range states
audio20_control_range_4_n_t(1) sampleFreqRng;

void (*usb_audio_in_callback)(void) = nullptr;

uint16_t startVal = 0;

uint16_t usb_audio_available() {
    return tud_audio_n_available(ITF_IX_SPEAKER);
}

void set_audio_in_callback(void (*f)()) {
    usb_audio_in_callback = f;
}

void usb_composite_init(void) {

    // Initialize USB hardware
    MX_USB_OTG_HS_Init();
    // Initialize TinyUSB
    tusb_rhport_init(BOARD_TUD_RHPORT, NULL);

    // Initialize audio parameters
    sampFreq = USB_AUDIO_SAMPLE_RATE;
    clkValid = 1;

    sampleFreqRng.wNumSubRanges = 1;
    sampleFreqRng.subrange[0].bMin = USB_AUDIO_SAMPLE_RATE;
    sampleFreqRng.subrange[0].bMax = USB_AUDIO_SAMPLE_RATE;
    sampleFreqRng.subrange[0].bRes = 0;

    // Initialize volume/mute
    for (int i = 0; i < CFG_TUD_AUDIO; i++) {
        for (int j = 0; j < CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1; j++) {
            usb::volume_db[i][j] = 0; // 0 dB
            usb::mute[i][j] = 0;      // Not muted
        }
    }

    // Initialize audio bridge
    usb_audio_dsp_bridge_init();

    // Flags the audio has to be bridged to USB for both interfaces
    // TODO: This should start false but somehow I wasn't able to detect the interface change
    usb_audio_dsp_bridge_start(ITF_IX_MICROPHONE);
    usb_audio_dsp_bridge_start(ITF_IX_SPEAKER);

    cdc_connected = false;
    msc_connected = false;

    // Start USB timer
    HAL_TIM_Base_Start_IT(&htim6);
}

extern "C" void TIM6_DAC_IRQHandler(void) {
    usb_composite_task();
    HAL_TIM_IRQHandler(&htim6);
}

// Check if USB cable is physically connected
bool usb_cable_connected() {
    // Check VBUS detection
    // For STM32F4 USB OTG HS, read VBUS sensing
    // return (USB_OTG_HS->GCCFG & USB_OTG_GCCFG_VBUSASEN) && (USB_OTG_HS->GOTGCTL & USB_OTG_GOTGCTL_BSVLD);
    // Check if VBUS is present (above ~4.4V threshold) Needs VBUS sensing enabled
    //     return (USB_OTG_HS->GOTGCTL & USB_OTG_GOTGCTL_BSVLD) != 0;
    // Read PB13 which is connected to USB_OTG_HS_VBUS
    return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13) == GPIO_PIN_SET;
}

bool usb_connected() {
    return tud_mounted() && usb_cable_connected();
}

static uint32_t usb_task_cnt = 0; // A counter to be able to "prescale" the audio streaming check
volatile bool usb_enabled = true;

// Call this to trigger re-enumeration with new descriptor
void usb_trigger_reenumeration(void) {

    usb_enabled = false;

    // Disconnect from bus
    tud_disconnect();

    // Wait a bit
    HAL_Delay(100);

    // Reconnect - will use new descriptor
    tud_connect();

    usb_enabled = true;
}

void usb_composite_task(void) {

    if (!usb_cable_connected()) {
        return;
    }

    if (!usb_enabled) {
        return;
    }
    // TinyUSB device task - must be called frequently
    tud_task_ext(1, false);

    // Audio-out processing. Commented-out if done in the audio DMA ISR.
    // usb_audio_process();

    // Audio-in processing
    bool check_usb_connection = (usb_task_cnt & 0x05) == 0;
    if (usb_audio_is_streaming(ITF_IX_SPEAKER, check_usb_connection) && usb_audio_in_callback) {

        uint16_t av_samples_in = usb_audio_available() / sizeof(uint16_t);
        if (av_samples_in >= DSP_BLOCK) {
            usb_audio_in_callback();
        }

        usb_task_cnt++;
    }
}

bool usb_cdc_connected(void) {
    // This is weak, but cdc_connected is true only when the host sends a DTR signal, and this is not guaranteed
    return cdc_connected || tud_cdc_connected();
}

bool usb_msc_connected(void) {
    return msc_connected;
}

//--------------------------------------------------------------------+
// CDC CALLBACKS - Your CAT Protocol
//--------------------------------------------------------------------+

// Invoked when CDC interface receives data from host
void tud_cdc_rx_cb(uint8_t itf) {
    (void)itf; // Interface index (we only have one CDC)

    uint8_t buf[64];
    uint32_t count = tud_cdc_n_read(itf, buf, sizeof(buf));

    if (count > 0) {
        // Forward to your existing CAT protocol handler
        cat_enqueue_command((char *)buf, count);
    }
}

// Invoked when a TX is complete and more data can be sent
void tud_cdc_tx_complete_cb(uint8_t itf) {
    (void)itf;
    // Can be used if you have a TX queue
}

// Invoked when line state changes (DTR/RTS)
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
    (void)itf;
    (void)rts;

    // DTR = Data Terminal Ready (PC has opened the port)
    // Warning! host does not always send the DTR signal
    cdc_connected = dtr;
}

// Invoked when line coding is changed (baud rate, etc)
void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const *p_line_coding) {
    (void)itf;
    (void)p_line_coding;
    // CAT protocol typically doesn't care about baud rate
}

// Function to send data via CDC (replaces HAL's CDC_Transmit_HS)
bool usb_cdc_transmit(const uint8_t *data, uint16_t len) {

    uint32_t sent = 0;
    while (sent < len) {
        uint32_t available = tud_cdc_write_available();
        if (available == 0) {
            tud_cdc_write_flush();
            continue;
        }

        uint32_t to_send = (len - sent) < available ? (len - sent) : available;
        uint32_t written = tud_cdc_write(data + sent, to_send);
        sent += written;
    }

    tud_cdc_write_flush();
    return true;
}

//--------------------------------------------------------------------+
// MSC CALLBACKS - Your SD Card Access
//--------------------------------------------------------------------+

// Invoked when received SCSI_CMD_INQUIRY
uint32_t tud_msc_inquiry2_cb(uint8_t lun, scsi_inquiry_resp_t *inquiry_resp, uint32_t bufsize) {
    (void)lun;
    (void)bufsize;
    const char vid[] = "A.Dust";
    const char pid[] = "EL24 USB storage";
    const char rev[] = "24";

    strncpy((char *)inquiry_resp->vendor_id, vid, 8);
    strncpy((char *)inquiry_resp->product_id, pid, 16);
    strncpy((char *)inquiry_resp->product_rev, rev, 4);

    return sizeof(scsi_inquiry_resp_t); // 36 bytes
}

// Invoked when received Test Unit Ready command
bool tud_msc_test_unit_ready_cb(uint8_t lun) {
    (void)lun;

    msc_connected = true;
    bool b = sd_card_is_ready();
    LOG("usb: tud_msc_test_unit_ready_cb | lun:%d : %d\n", lun, b);
    return b;
}

bool tud_msc_is_writable_cb(uint8_t lun) {
    (void)lun;

    LOG("msc: tud_msc_is_writable_cb: 1\n");
    return true;
}

// Invoked when received SCSI_CMD_READ_CAPACITY_10 and SCSI_CMD_READ_FORMAT_CAPACITY
void tud_msc_capacity_cb(uint8_t lun, uint32_t *block_count, uint16_t *block_size) {
    (void)lun;

    *block_count = sd_card_get_block_count();
    *block_size = MSD_BLOCK_SIZE;

    LOG("msc: tud_msc_capacity_cb | blocks: %lu | size: %d\n", *block_count, *block_size);
}

// Invoked when received Start Stop Unit command
bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject) {
    (void)lun;
    (void)power_condition;
    (void)start;
    (void)load_eject;

    return true;
}

// Callback invoked when received READ10 command
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize) {
    (void)lun;
    (void)offset; // TinyUSB handles offset internally

    // Calculate number of blocks to read
    uint32_t block_count = bufsize / MSD_BLOCK_SIZE;
    int32_t ret = -1;

    // Read from SD card using your existing function
    int read_status = sd_card_read_blocks(lba, (uint8_t *)buffer, block_count);

    if (read_status == 0) {
        ret = bufsize;
    } else {
        LOG("msc: tud_msc_read10_cb | read blocks (lun: %d,lba: %d,offset: %d", lun, lba, offset);
        LOG_RAW(",n_blocks: %d,ret: 0x%x) : %d\n", block_count, read_status, ret);
    }

    return ret; // Error
}

// Callback invoked when received WRITE10 command
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize) {
    (void)lun;
    (void)offset;

    uint32_t block_count = bufsize / MSD_BLOCK_SIZE;
    int32_t ret = -1;

    if (sd_card_write_blocks(lba, buffer, block_count) == 0) {
        ret = bufsize;
    } else {
        LOG("msc: tud_msc_write10_cb | write blocks (lun: %d,lba: %d", lun, lba);
        LOG_RAW(",offset: %d,n_blocks: %d) : %d\n", offset, block_count, ret);
    }

    return ret;
}

// Callback invoked when WRITE10 command is completed
void tud_msc_write10_complete_cb(uint8_t lun) {
    (void)lun;
    // LOG("msc: tud_msc_write10_complete_cb: %d\n", lun);
    // Optional: flush SD card cache if needed
}

// Callback invoked when received an SCSI command not in built-in list
int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void *buffer, uint16_t bufsize) {
    void const *response = NULL;
    int32_t resplen = 0;

    LOG("msc: tud_msc_scsi_sb | command: %d\n", scsi_cmd[0]);
    switch (scsi_cmd[0]) {
        default:
            // Set Sense = Invalid Command Operation
            tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
            resplen = -1;
            break;
    }

    if (resplen > bufsize) {
        resplen = bufsize;
    }

    if (response && (resplen > 0)) {
        memcpy(buffer, response, resplen);
    }

    return resplen;
}

//--------------------------------------------------------------------+
// DEVICE CALLBACKS
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void) {
    // Device connected to host
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
    cdc_connected = false;
    msc_connected = false;
    usb_audio_dsp_bridge_stop(ITF_IX_MICROPHONE);
    usb_audio_dsp_bridge_stop(ITF_IX_SPEAKER);
}

// Invoked when usb bus is suspended
void tud_suspend_cb(bool remote_wakeup_en) {
    (void)remote_wakeup_en;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {
}

// Must be user-implemented
uint32_t tusb_time_millis_api(void) {
    return HAL_GetTick();
}

//--------------------------------------------------------------------+
// Audio helpers
//--------------------------------------------------------------------+

uint8_t to_audio_interface_index(uint8_t tud_itf_number) {

    // Includes cases for SREAMING iterfaces but the host should ask for volume and mute only in control interfaces
    // Just in case
    if (usb_get_msc_enabled()) {
        switch (tud_itf_number) {
            case ITF_NUM_AUDIO_CONTROL:
            case ITF_NUM_AUDIO_STREAMING:
                return ITF_IX_MICROPHONE;
                break;
            case ITF_NUM_SPK_CONTROL:
            case ITF_NUM_SPK_STREAMING:
                return ITF_IX_SPEAKER;
                break;
            default:
                TU_BREAKPOINT();
                return 0;
                break;
        }
    } else {
        switch (tud_itf_number) {
            case ITF_NUM_AUDIO_CONTROL_NO_MSC:
            case ITF_NUM_AUDIO_STREAMING_NO_MSC:
                return ITF_IX_MICROPHONE;
                break;

            case ITF_NUM_SPK_CONTROL_NO_MSC:
            case ITF_NUM_SPK_STREAMING_NO_MSC:

                return ITF_IX_SPEAKER;
                break;
            default:
                TU_BREAKPOINT();
                return 0;
                break;
        }
    }
}

//--------------------------------------------------------------------+
// Audio callback API implementations
//--------------------------------------------------------------------+

// Invoked when audio class specific set request received for an EP
bool tud_audio_set_req_ep_cb(uint8_t rhport, tusb_control_request_t const *p_request, uint8_t *pBuff) {
    (void)rhport;
    (void)pBuff;

    // We do not support any set range requests here, only current value requests
    TU_VERIFY(p_request->bRequest == AUDIO20_CS_REQ_CUR);

    // Page 91 in UAC2 specification
    uint8_t channelNum = TU_U16_LOW(p_request->wValue);
    uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
    uint8_t ep = TU_U16_LOW(p_request->wIndex);

    (void)channelNum;
    (void)ctrlSel;
    (void)ep;

    return false; // Yet not implemented
}

// Invoked when audio class specific set request received for an interface (not implemented)
bool tud_audio_set_req_itf_cb(uint8_t rhport, tusb_control_request_t const *p_request, uint8_t *pBuff) {
    (void)rhport;
    (void)pBuff;

    // We do not support any set range requests here, only current value requests
    TU_VERIFY(p_request->bRequest == AUDIO20_CS_REQ_CUR);

    // Page 91 in UAC2 specification
    uint8_t channelNum = TU_U16_LOW(p_request->wValue);
    uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
    uint8_t itf = TU_U16_LOW(p_request->wIndex);

    (void)channelNum;
    (void)ctrlSel;
    (void)itf;

    return false; // Yet not implemented
}

static float db_to_linear(int16_t db) {

    float gain = powf(10.0f, db / 20.0f);
    return gain;
}

static int16_t map_range(int16_t x, int16_t in_min, int16_t in_max, int16_t out_min, int16_t out_max) {
    int32_t num = (int32_t)(x - in_min) * (out_max - out_min);
    int32_t den = (in_max - in_min);

    if (den == 0) {
        return out_min;
    }

    return (int16_t)(out_min + num / den);
}

static int16_t host_to_device_vol_map(int16_t x) {
    return map_range(x, host_min_db, host_max_db, vol_min_db, vol_max_db);
}

static int16_t device_to_host_vol_map(int16_t x) {
    return map_range(x, vol_min_db, vol_max_db, host_min_db, host_max_db);
}

// Invoked when audio class specific set request received for an entity
bool tud_audio_set_req_entity_cb(uint8_t rhport, tusb_control_request_t const *p_request, uint8_t *pBuff) {
    (void)rhport;

    // Page 91 in UAC2 specification
    uint8_t channelNum = TU_U16_LOW(p_request->wValue);
    uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
    uint8_t itf = TU_U16_LOW(p_request->wIndex);
    uint8_t itf_ix = to_audio_interface_index(itf);
    uint8_t entityID = TU_U16_HIGH(p_request->wIndex);

    // We do not support any set range requests here, only current value requests
    TU_VERIFY(p_request->bRequest == AUDIO20_CS_REQ_CUR);

    int16_t vol_raw;
    // If request is for our feature unit
    if (entityID == 2) {
        switch (ctrlSel) {
            case AUDIO20_FU_CTRL_MUTE:
                // Request uses format layout 1
                TU_VERIFY(p_request->wLength == sizeof(audio20_control_cur_1_t));

                usb::mute[itf_ix][channelNum] = ((audio20_control_cur_1_t *)pBuff)->bCur;

                LOG("usb: set mute: %d of channel %u itf %u\n", usb::mute[itf_ix][channelNum], channelNum, itf);
                return true;

            case AUDIO20_FU_CTRL_VOLUME:
                // Request uses format layout 2
                TU_VERIFY(p_request->wLength == sizeof(audio20_control_cur_2_t));

                vol_raw = ((audio20_control_cur_2_t *)pBuff)->bCur;

                usb::volume_db[itf_ix][channelNum] = host_to_device_vol_map(vol_raw);
                usb::volume_factor[itf_ix][channelNum] = db_to_linear(usb::volume_db[itf_ix][channelNum]);

                LOG("usb: set volume: %d (raw: %d) dB of channel %u itf %u\n", usb::volume_db[itf_ix][channelNum], vol_raw, channelNum, itf);
                return true;

                // Unknown/Unsupported control
            default:
                TU_BREAKPOINT();
                return false;
        }
    }
    return false; // Yet not implemented
}

// Invoked when audio class specific get request received for an EP
bool tud_audio_get_req_ep_cb(uint8_t rhport, tusb_control_request_t const *p_request) {
    (void)rhport;

    // Page 91 in UAC2 specification
    uint8_t channelNum = TU_U16_LOW(p_request->wValue);
    uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
    uint8_t ep = TU_U16_LOW(p_request->wIndex);

    (void)channelNum;
    (void)ctrlSel;
    (void)ep;

    //	return tud_control_xfer(rhport, p_request, &tmp, 1);

    return false; // Yet not implemented
}

// Invoked when audio class specific get request received for an interface
bool tud_audio_get_req_itf_cb(uint8_t rhport, tusb_control_request_t const *p_request) {
    (void)rhport;

    // Page 91 in UAC2 specification
    uint8_t channelNum = TU_U16_LOW(p_request->wValue);
    uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
    uint8_t itf = TU_U16_LOW(p_request->wIndex);

    (void)channelNum;
    (void)ctrlSel;
    (void)itf;

    return false; // Yet not implemented
}

// Invoked when audio class specific get request received for an entity
bool tud_audio_get_req_entity_cb(uint8_t rhport, tusb_control_request_t const *p_request) {
    (void)rhport;

    // Page 91 in UAC2 specification
    uint8_t channelNum = TU_U16_LOW(p_request->wValue);
    uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
    uint8_t itf = TU_U16_LOW(p_request->wIndex);

    uint8_t entityID = TU_U16_HIGH(p_request->wIndex);

    // Input terminal (Microphone input)
    if (entityID == 1) {
        switch (ctrlSel) {
            case AUDIO20_TE_CTRL_CONNECTOR: {
                // The terminal connector control only has a get request with only the CUR attribute.
                audio20_desc_channel_cluster_t ret;

                // Those are dummy values for now
                ret.bNrChannels = 1;
                ret.bmChannelConfig = (audio20_channel_config_t)0;
                ret.iChannelNames = 0;

                LOG("usb: get terminal connector itf %d\n", itf);

                return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request, (void *)&ret, sizeof(ret));
            } break;

                // Unknown/Unsupported control selector
            default:
                TU_BREAKPOINT();
                return false;
        }
    }

    uint16_t vol_value;
    // Feature unit
    if (entityID == 2) {
        uint8_t itf_ix = to_audio_interface_index(itf);
        switch (ctrlSel) {
            case AUDIO20_FU_CTRL_MUTE:
                // Audio control mute cur parameter block consists of only one byte - we thus can send it right away
                // There does not exist a range parameter block for mute
                LOG("usb: get mute of channel %u itf %u = %u\n", channelNum, itf, usb::mute[itf_ix][channelNum]);
                return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request, &usb::mute[itf_ix][channelNum], 1);

            case AUDIO20_FU_CTRL_VOLUME:
                switch (p_request->bRequest) {
                    case AUDIO20_CS_REQ_CUR:

                        vol_value = device_to_host_vol_map(usb::volume_db[itf_ix][channelNum]);
                        LOG("usb: get volume of channel %u itf %u = %d\n", channelNum, itf, usb::volume_db[itf_ix][channelNum]);
                        return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request, &vol_value, sizeof(usb::volume_db[itf_ix][channelNum]));

                    case AUDIO20_CS_REQ_RANGE:

                        audio20_control_range_2_n_t(1) ret;

                        // NOTE: STM32 is already little-endian so no byte swaps is needed
                        // NOTE: Windows seem to ignore UAC 2.0 specs that say the values are 1/256 fixed point or I am missing something in the
                        // descriptor. Anyway, this what's working now.
                        // Also, the slider in windows seems to always represent -90 to 90. If I set the range to be -50 to 0, the half right of the slider
                        // does not cause the volume to increase from 0 (slider center)
                        ret.wNumSubRanges = 1;
                        ret.subrange[0].bMin = host_min_db;
                        ret.subrange[0].bMax = host_max_db;
                        ret.subrange[0].bRes = 1;
                        LOG("usb: get volume range of channel %d itf %u", channelNum, itf);
                        LOG_RAW(" = min:%d, max:%d, res:%d\n", ret.subrange[0].bMin, ret.subrange[0].bMax, ret.subrange[0].bRes);
                        return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request, (void *)&ret, sizeof(ret));

                        // Unknown/Unsupported control
                    default:
                        TU_BREAKPOINT();
                        return false;
                }
                break;

                // Unknown/Unsupported control
            default:
                TU_BREAKPOINT();
                return false;
        }
    }

    // Clock Source unit
    if (entityID == 4) {
        switch (ctrlSel) {
            case AUDIO20_CS_CTRL_SAM_FREQ:
                // channelNum is always zero in this case
                switch (p_request->bRequest) {
                    case AUDIO20_CS_REQ_CUR:
                        LOG("usb: get sample freq itf %d: %d.\n", itf, sampFreq);
                        return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request, &sampFreq, sizeof(sampFreq));

                    case AUDIO20_CS_REQ_RANGE:
                        LOG("usb: get sample freq. range itf %d\n", itf);
                        return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request, &sampleFreqRng, sizeof(sampleFreqRng));

                        // Unknown/Unsupported control
                    default:
                        TU_BREAKPOINT();
                        return false;
                }
                break;

            case AUDIO20_CS_CTRL_CLK_VALID:
                // Only cur attribute exists for this request
                LOG("usb: get sample freq. valid itf %d\n", itf);
                return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request, &clkValid, sizeof(clkValid));

            // Unknown/Unsupported control
            default:
                TU_BREAKPOINT();
                return false;
        }
    }

    LOG("usb: unsupported entit %d on itf %d\n", entityID, itf);
    return false; // Yet not implemented
}

bool tud_audio_set_itf_cb(uint8_t rhport, tusb_control_request_t const *p_request) {
    (void)rhport;

    uint8_t itf = tu_u16_low(tu_le16toh(p_request->wIndex));
    uint8_t alt = tu_u16_low(tu_le16toh(p_request->wValue));

    LOG("usb: set interface %d -> alt %d\n", itf, alt);

    uint8_t itf_ix = to_audio_interface_index(itf);

    if (itf_ix == ITF_IX_MICROPHONE) {
        if (alt == 1) {
            // Audio streaming START
            LOG("usb: audio mic STREAMING\n");
            usb_audio_dsp_bridge_start(itf_ix);
        } else {
            // Audio streaming STOP
            LOG("usb: audio mic IDLE\n");
            //  usb_audio_dsp_bridge_stop();
        }
    } else {
        if (alt == 1) {
            // Audio streaming START
            LOG("usb: audio speaker STREAMING\n");
            usb_audio_dsp_bridge_start(itf_ix);
        } else {
            // Audio streaming STOP
            LOG("usb: audio speaker IDLE\n");
            //  usb_audio_dsp_bridge_stop();
        }
    }

    return true;
}

// Called when the iterface is closed
bool tud_audio_set_itf_close_ep_cb(uint8_t rhport, tusb_control_request_t const *p_request) {
    (void)rhport;

    uint8_t itf = tu_u16_low(tu_le16toh(p_request->wIndex));
    uint8_t itf_ix = to_audio_interface_index(itf);
    LOG("usb: audio interface %d closed\n", itf);
    usb_audio_dsp_bridge_stop(itf_ix);

    return true;
}
