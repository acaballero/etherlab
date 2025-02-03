//
// Created by Angel Dust on 02/07/2024.
//

#include "signal_generator.h"
#include "config.h"
#include "blocks_common.h"

static const int8_t sine_table_i8[LUT_SIZE] = {
    0,    2,    5,    8,    12,   15,   18,   21,   24,   27,   30,   33,   36,   39,   42,   45,   48,   51,   54,   57,   59,   62,   65,   67,   70,   73,
    75,   78,   80,   83,   85,   87,   90,   92,   94,   96,   98,   100,  102,  104,  105,  107,  109,  110,  112,  113,  115,  116,  117,  118,  120,  121,
    121,  122,  123,  124,  125,  125,  126,  126,  126,  127,  127,  127,  127,  127,  127,  127,  126,  126,  126,  125,  125,  124,  123,  122,  121,  121,
    120,  118,  117,  116,  115,  113,  112,  110,  109,  107,  105,  104,  102,  100,  98,   96,   94,   92,   90,   87,   85,   83,   80,   78,   75,   73,
    70,   67,   65,   62,   59,   57,   54,   51,   48,   45,   42,   39,   36,   33,   30,   27,   24,   21,   18,   15,   12,   8,    5,    2,    0,    -3,
    -6,   -9,   -13,  -16,  -19,  -22,  -25,  -28,  -31,  -34,  -37,  -40,  -43,  -46,  -49,  -52,  -55,  -58,  -60,  -63,  -66,  -68,  -71,  -74,  -76,  -79,
    -81,  -84,  -86,  -88,  -91,  -93,  -95,  -97,  -99,  -101, -103, -105, -106, -108, -110, -111, -113, -114, -116, -117, -118, -119, -121, -122, -122, -123,
    -124, -125, -126, -126, -127, -127, -127, -128, -128, -128, -128, -128, -128, -128, -127, -127, -127, -126, -126, -125, -124, -123, -122, -122, -121, -119,
    -118, -117, -116, -114, -113, -111, -110, -108, -106, -105, -103, -101, -99,  -97,  -95,  -93,  -91,  -88,  -86,  -84,  -81,  -79,  -76,  -74,  -71,  -68,
    -66,  -63,  -60,  -58,  -55,  -52,  -49,  -46,  -43,  -40,  -37,  -34,  -31,  -28,  -25,  -22,  -19,  -16,  -13,  -9,   -6,   -3};

void SignalGenerator::init() { tone_delta = (uint32_t)(((float)(LUT_SIZE * frequency) / (float)sample_rate) * (1 << 24)); }

adc_type SignalGenerator::get_sample(uint32_t phase) {
    int8_t a;
    int8_t sample;

    switch (tone_shape) {

        default:
        case SIGNAL_SHAPE_SIN:
            sample = (sine_table_i8[(phase & 0xFF000000) >> 24]);
            break;
        case SIGNAL_SHAPE_TRI:
            a = (phase & 0xFF000000) >> 24;
            sample = (a & 0x80) ? ((a << 1) ^ 0xFF) - 0x80 : (a << 1) + 0x80;
            break;
        case SIGNAL_SHAPE_SAW_UP:
            sample = ((phase & 0xFF000000) >> 24);
            break;
        case SIGNAL_SHAPE_SAW_DOWN:
            sample = ((phase & 0xFF000000) >> 24) ^ 0xFF;
            break;
    }

    return sample;
}

void SignalGenerator::get_sample(adc_type &sample) {
    tone_phase += tone_delta;
    sample = get_sample(tone_phase);
}

void SignalGenerator::get_complex_sample(complex_t &sample) {

    //        if (!sample_count && auto_off) {
    //            txprogress_message.done = true;
    //            shared_memory.application_queue.push(txprogress_message);
    //        } else
    //            sample_count--;

    tone_phase += tone_delta;

    /** Apply FM
    // delta = sample * fm_delta;

    //phase += delta;
    //sphase = phase + ((SINE_LUT_SIZE >> 2) << 24); // 90 deg

    //re = (sine_table_i8[(sphase & 0xFF000000) >> 24]);
    //im = (sine_table_i8[(phase & 0xFF000000) >> 24]); **/

    sample.r = get_sample(tone_phase);
    sample.i = get_sample(tone_phase + ((LUT_SIZE >> 2) << 24)); // 90 deg
}

void SignalGenerator::get_block(buffer_t<complex_t> &buffer) {
    complex_t sample;
    for (size_t i = 0; i < buffer.count; i++) {
        get_complex_sample(sample);
        buffer.p[i].i = sample.i;
        buffer.p[i].r = sample.r;
    }
}

void SignalGenerator::set_shape(SIGNAL_SHAPE shape) {
    tone_shape = shape;
    init();
}

void SignalGenerator::set_config(uint32_t f, uint32_t sr) {
    frequency = f;
    sample_rate = sr;
    init();
}
