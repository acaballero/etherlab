/**
 * usb_composite_device.c
 *
 * Complete TinyUSB composite device implementation
 * Replaces all HAL USB code with TinyUSB for CDC+MSC+Audio
 */

#include "../../../lib/tinyusb/src/tusb.h"
#include "device/usbd.h"
#include "hw/stm32f4xx/usb.h"
#include "tinyusb/tusb_config.h"
#include "usb_composite_device.h"
#include "usb_audio_dsp_bridge.h"
#include "io/cat_if.h"
#include "hw/stm32_hal.h"
#include <string.h>

// SD card functions (you'll need to provide these)
extern int sd_card_read_blocks(uint32_t lba, uint8_t *buffer, uint32_t block_count);
extern int sd_card_write_blocks(uint32_t lba, const uint8_t *buffer, uint32_t block_count);
extern uint32_t sd_card_get_block_count(void);
extern bool sd_card_is_ready(void);

// USB device state
static bool cdc_connected = false;
static bool msc_connected = false;

//--------------------------------------------------------------------+
// INITIALIZATION
//--------------------------------------------------------------------+

void usb_composite_init(void) {

    // Initialize USB hardware
    MX_USB_OTG_HS_Init();
    // Initialize TinyUSB
    tusb_rhport_init(BOARD_TUD_RHPORT, NULL);

    // Initialize audio bridge
    usb_audio_dsp_bridge_init();
    usb_audio_dsp_bridge_start();

    cdc_connected = false;
    msc_connected = false;
}

void usb_composite_task(void) {
    // TinyUSB device task - must be called frequently
    tud_task_ext(1, false);

    // Audio processing
    usb_audio_process();
}

bool usb_composite_cdc_connected(void) {
    return cdc_connected;
}

bool usb_composite_msc_connected(void) {
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
    cdc_connected = dtr;
}

// Invoked when line coding is changed (baud rate, etc)
void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const *p_line_coding) {
    (void)itf;
    (void)p_line_coding;
    // CAT protocol typically doesn't care about baud rate
}

// Function to send data via CDC (replaces CDC_Transmit_HS)
bool usb_cdc_transmit(const uint8_t *data, uint16_t len) {
    if (!cdc_connected || !tud_cdc_connected()) {
        return false;
    }

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
void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]) {

    if (!usb_get_msc_enabled()) { // Does nothing if MSC is explicitly disabled
        return;
    }

    (void)lun;

    const char vid[] = "Angel Dust";
    const char pid[] = "Etherlab EL24 SDR Transceiver";
    const char rev[] = "1.0";

    memcpy(vendor_id, vid, strlen(vid));
    memcpy(product_id, pid, strlen(pid));
    memcpy(product_rev, rev, strlen(rev));
}

// Invoked when received Test Unit Ready command
bool tud_msc_test_unit_ready_cb(uint8_t lun) {
    (void)lun;

    msc_connected = true;
    return sd_card_is_ready();
}

// Invoked when received SCSI_CMD_READ_CAPACITY_10 and SCSI_CMD_READ_FORMAT_CAPACITY
void tud_msc_capacity_cb(uint8_t lun, uint32_t *block_count, uint16_t *block_size) {
    (void)lun;

    *block_count = sd_card_get_block_count();
    *block_size = 512;
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
    uint32_t block_count = bufsize / 512;

    // Read from SD card using your existing function
    if (sd_card_read_blocks(lba, (uint8_t *)buffer, block_count) == 0) {
        return bufsize;
    }

    return -1; // Error
}

// Callback invoked when received WRITE10 command
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize) {
    (void)lun;
    (void)offset;

    uint32_t block_count = bufsize / 512;

    if (sd_card_write_blocks(lba, buffer, block_count) == 0) {
        return bufsize;
    }

    return -1;
}

// Callback invoked when WRITE10 command is completed
void tud_msc_write10_complete_cb(uint8_t lun) {
    (void)lun;
    // Optional: flush SD card cache if needed
}

// Callback invoked when received an SCSI command not in built-in list
int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void *buffer, uint16_t bufsize) {
    void const *response = NULL;
    int32_t resplen = 0;

    switch (scsi_cmd[0]) {
        default:
            // Set Sense = Invalid Command Operation
            tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
            resplen = -1;
            break;
    }

    if (resplen > bufsize)
        resplen = bufsize;

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
    usb_audio_dsp_bridge_stop();
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
// Application Callback API Implementations
//--------------------------------------------------------------------+

bool mute[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1];       // +1 for master channel 0
uint16_t volume[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1]; // +1 for master channel 0
uint32_t sampFreq;
uint8_t clkValid;

// Range states
audio20_control_range_4_n_t(1) sampleFreqRng;

uint16_t startVal = 0;

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

// Invoked when audio class specific set request received for an interface
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

// Invoked when audio class specific set request received for an entity
bool tud_audio_set_req_entity_cb(uint8_t rhport, tusb_control_request_t const *p_request, uint8_t *pBuff) {
    (void)rhport;

    // Page 91 in UAC2 specification
    uint8_t channelNum = TU_U16_LOW(p_request->wValue);
    uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
    uint8_t itf = TU_U16_LOW(p_request->wIndex);
    uint8_t entityID = TU_U16_HIGH(p_request->wIndex);

    (void)itf;

    // We do not support any set range requests here, only current value requests
    TU_VERIFY(p_request->bRequest == AUDIO20_CS_REQ_CUR);

    // If request is for our feature unit
    if (entityID == 2) {
        switch (ctrlSel) {
            case AUDIO20_FU_CTRL_MUTE:
                // Request uses format layout 1
                TU_VERIFY(p_request->wLength == sizeof(audio20_control_cur_1_t));

                mute[channelNum] = ((audio20_control_cur_1_t *)pBuff)->bCur;

                TU_LOG2("    Set Mute: %d of channel: %u\r\n", mute[channelNum], channelNum);
                return true;

            case AUDIO20_FU_CTRL_VOLUME:
                // Request uses format layout 2
                TU_VERIFY(p_request->wLength == sizeof(audio20_control_cur_2_t));

                volume[channelNum] = (uint16_t)((audio20_control_cur_2_t *)pBuff)->bCur;

                TU_LOG2("    Set Volume: %d dB of channel: %u\r\n", volume[channelNum], channelNum);
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
    // uint8_t itf = TU_U16_LOW(p_request->wIndex); 			// Since we have only one audio function implemented, we do not need the itf value
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

                TU_LOG2("    Get terminal connector\r\n");

                return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request, (void *)&ret, sizeof(ret));
            } break;

                // Unknown/Unsupported control selector
            default:
                TU_BREAKPOINT();
                return false;
        }
    }

    // Feature unit
    if (entityID == 2) {
        switch (ctrlSel) {
            case AUDIO20_FU_CTRL_MUTE:
                // Audio control mute cur parameter block consists of only one byte - we thus can send it right away
                // There does not exist a range parameter block for mute
                TU_LOG2("    Get Mute of channel: %u\r\n", channelNum);
                return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request, &mute[channelNum], 1);

            case AUDIO20_FU_CTRL_VOLUME:
                switch (p_request->bRequest) {
                    case AUDIO20_CS_REQ_CUR:
                        TU_LOG2("    Get Volume of channel: %u\r\n", channelNum);
                        return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request, &volume[channelNum], sizeof(volume[channelNum]));

                    case AUDIO20_CS_REQ_RANGE:
                        TU_LOG2("    Get Volume range of channel: %u\r\n", channelNum);

                        // Copy values - only for testing - better is version below
                        audio20_control_range_2_n_t(1) ret;

                        ret.wNumSubRanges = 1;
                        ret.subrange[0].bMin = -90; // -90 dB
                        ret.subrange[0].bMax = 90;  // +90 dB
                        ret.subrange[0].bRes = 1;   // 1 dB steps

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
                        TU_LOG2("    Get Sample Freq.\r\n");
                        return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request, &sampFreq, sizeof(sampFreq));

                    case AUDIO20_CS_REQ_RANGE:
                        TU_LOG2("    Get Sample Freq. range\r\n");
                        return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request, &sampleFreqRng, sizeof(sampleFreqRng));

                        // Unknown/Unsupported control
                    default:
                        TU_BREAKPOINT();
                        return false;
                }
                break;

            case AUDIO20_CS_CTRL_CLK_VALID:
                // Only cur attribute exists for this request
                TU_LOG2("    Get Sample Freq. valid\r\n");
                return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request, &clkValid, sizeof(clkValid));

            // Unknown/Unsupported control
            default:
                TU_BREAKPOINT();
                return false;
        }
    }

    TU_LOG2("  Unsupported entity: %d\r\n", entityID);
    return false; // Yet not implemented
}

bool tud_audio_set_itf_close_ep_cb(uint8_t rhport, tusb_control_request_t const *p_request) {
    (void)rhport;
    (void)p_request;
    startVal = 0;

    return true;
}
