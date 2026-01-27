//
// Enhanced HilbertTransform implementation
//
#include "dsp_hilbert.h"
#include "../../../lib/DspFilters/include/Dsp.h"
#include "../../../lib/DspFilters/include/ChebyshevI.h"
#include "../../../lib/DspFilters/include/State.h"
#include "../../../lib/DspFilters/include/Cascade.h"
#include "../../../lib/DspFilters/include/Filter.h"
#include "../../status.h"

namespace dsp {

// ============================================================================
// HILBERT TRANSFORM
// ============================================================================

HilbertTransform::HilbertTransform() {
    n = 0;
    configured = false;
}

bool HilbertTransform::configure(float32_t *coeffs) {
    // Configure all three filters with the same coefficients
    sos_input.configure(coeffs);
    sos_i.configure(coeffs);
    sos_q.configure(coeffs);

    configured = true;
    return true;
}

bool HilbertTransform::configure(uint32_t sr) {
    // Check if reconfiguration needed
    if (configured && sample_rate == sr) {
        return true;
    }

    sample_rate = sr;

    // For Hilbert transform we use half-band filter at fs/4
    // Higher rate/cutoff ratios require more stages
    const int order = 10; // 5 biquad stages
    float32_t cutoff_freq = sample_rate / 4.0f;

    // Design Butterworth lowpass
    Dsp::SimpleFilter<Dsp::Butterworth::LowPass<order>, 1, Dsp::DirectFormI> filter;
    filter.setup(order, (double)sample_rate, (double)cutoff_freq);

    // Get cascade stages
    Dsp::Cascade::Storage st = filter.getCascadeStorage();
    int n_stages = filter.getNumStages();

    if (n_stages != 5) {
        LOG("HilbertTransform::configure: Expected 5 stages, got %d\n", n_stages);
        return false;
    }

    // Convert to CMSIS format

    for (int stage = 0; stage < n_stages; stage++) {
        const auto &dg = st.stageArray[stage];
        int offset = stage * 5;

        // Normalize by a0 and negate a1, a2
        coeffs[offset + 0] = dg.m_b0 / dg.m_a0;
        coeffs[offset + 1] = dg.m_b1 / dg.m_a0;
        coeffs[offset + 2] = dg.m_b2 / dg.m_a0;
        coeffs[offset + 3] = -dg.m_a1 / dg.m_a0;
        coeffs[offset + 4] = -dg.m_a2 / dg.m_a0;

        // LOG THE COEFFICIENTS
        LOG("Stage %d: b=[%.6f, %.6f, %.6f] ", stage, coeffs[offset + 0], coeffs[offset + 1], coeffs[offset + 2]);
        LOG_RAW("a=[%.6f, %.6f]\n", coeffs[offset + 3], coeffs[offset + 4]);
    }

    // Configure filters
    configure(coeffs);

    LOG("HilbertTransform configured: fs=%u, fc=%u\n", sample_rate, (uint32_t)cutoff_freq);
    configured = true;
    return true;
}

void HilbertTransform::execute(float in, float &out_i, float &out_q) {
    // Synthesized Hilbert Transform using fs/4 frequency shifting
    // Input -> LPF -> fs/4 shift -> I and Q paths -> LPF each -> output

    float a = 0, b = 0;

    // Anti-aliasing LPF at fs/4
    float in_filtered = sos_input.execute(in);

    // fs/4 frequency shift (rotation by n*90°)
    switch (n) {
        case 0:
            a = in_filtered;
            b = 0;
            break;
        case 1:
            a = 0;
            b = -in_filtered;
            break;
        case 2:
            a = -in_filtered;
            b = 0;
            break;
        case 3:
            a = 0;
            b = in_filtered;
            break;
    }

    // Filter I and Q paths
    float i = sos_i.execute(a) * 2.0f;
    float q = sos_q.execute(b) * 2.0f;

    // Rotate back
    switch (n) {
        case 0:
            out_i = i;
            out_q = q;
            break;
        case 1:
            out_i = -q;
            out_q = i;
            break;
        case 2:
            out_i = -i;
            out_q = -q;
            break;
        case 3:
            out_i = q;
            out_q = -i;
            break;
    }

    n = (n + 1) % 4;
}

void HilbertTransform::reset() {
    sos_input.reset();
    sos_i.reset();
    sos_q.reset();
    n = 0;
}

// ============================================================================
// REAL TO COMPLEX
// ============================================================================

RealToComplex::RealToComplex() {
    n = 0;
    configured_ = false;
}

bool RealToComplex::configure(uint32_t sample_rate) {
    if (configured_ && sample_rate == sample_rate_) {
        return true;
    }

    sample_rate_ = sample_rate;

    const int order = 10;

    // Full-band LPF for input (fc = fs/2)
    Dsp::SimpleFilter<Dsp::Butterworth::LowPass<order>, 1, Dsp::DirectFormI> full_band_filter;
    full_band_filter.setup(order, (double)sample_rate, (double)(sample_rate / 2.0f));

    // Quarter-band LPF for magnitude (fc = fs/4)
    Dsp::SimpleFilter<Dsp::Butterworth::LowPass<order>, 1, Dsp::DirectFormI> quarter_band_filter;
    quarter_band_filter.setup(order, (double)sample_rate, (double)(sample_rate / 4.0f));

    // Configure full-band filters (input, i, q)
    Dsp::Cascade::Storage st_full = full_band_filter.getCascadeStorage();
    int n_stages = full_band_filter.getNumStages();

    if (n_stages != 5) {
        LOG("RealTo cComplex::configure: Expected 5 stages, got %d\n", n_stages);
        return false;
    }

    float32_t coeffs_full[25];
    for (int stage = 0; stage < n_stages; stage++) {
        const auto &dg = st_full.stageArray[stage];
        int offset = stage * 5;
        coeffs_full[offset + 0] = dg.m_b0 / dg.m_a0;
        coeffs_full[offset + 1] = dg.m_b1 / dg.m_a0;
        coeffs_full[offset + 2] = dg.m_b2 / dg.m_a0;
        coeffs_full[offset + 3] = -dg.m_a1 / dg.m_a0;
        coeffs_full[offset + 4] = -dg.m_a2 / dg.m_a0;
    }

    sos_input.configure(coeffs_full);
    sos_i.configure(coeffs_full);
    sos_q.configure(coeffs_full);

    // Configure quarter-band filter for magnitude
    Dsp::Cascade::Storage st_quarter = quarter_band_filter.getCascadeStorage();
    float32_t coeffs_quarter[25];
    for (int stage = 0; stage < n_stages; stage++) {
        const auto &dg = st_quarter.stageArray[stage];
        int offset = stage * 5;
        coeffs_quarter[offset + 0] = dg.m_b0 / dg.m_a0;
        coeffs_quarter[offset + 1] = dg.m_b1 / dg.m_a0;
        coeffs_quarter[offset + 2] = dg.m_b2 / dg.m_a0;
        coeffs_quarter[offset + 3] = -dg.m_a1 / dg.m_a0;
        coeffs_quarter[offset + 4] = -dg.m_a2 / dg.m_a0;
    }

    sos_mag_sq.configure(coeffs_quarter);

    configured_ = true;
    LOG("RealToComplex configured: fs=%u\n", sample_rate);
    return true;
}

void RealToComplex::execute(float in, float &out_mag_sq_lpf) {
    float a = 0, b = 0;
    float out_i = 0, out_q = 0;

    // Full-band anti-aliasing LPF
    float in_filtered = sos_input.execute(in);

    // fs/4 frequency shift
    switch (n) {
        case 0:
            a = in_filtered;
            b = 0;
            break;
        case 1:
            a = 0;
            b = -in_filtered;
            break;
        case 2:
            a = -in_filtered;
            b = 0;
            break;
        case 3:
            a = 0;
            b = in_filtered;
            break;
    }

    // Filter I and Q paths
    float i = sos_i.execute(a);
    float q = sos_q.execute(b);

    // Shift down -fs/4
    switch (n) {
        case 0:
            out_i = i;
            out_q = q;
            break;
        case 1:
            out_i = -q;
            out_q = i;
            break;
        case 2:
            out_i = -i;
            out_q = -q;
            break;
        case 3:
            out_i = q;
            out_q = -i;
            break;
    }

    n = (n + 1) % 4;

    // Compute cross-magnitude (for APT satellite signals)
    float out_mag_sq = out_i * out_q; // Cross product approximation

    // LPF to remove subcarrier
    out_mag_sq_lpf = sos_mag_sq.execute(out_mag_sq) * 2.0f;

    // Normalize and compress
    out_mag_sq_lpf /= 32768.0f;
    if (out_mag_sq_lpf > 1.0f) {
        out_mag_sq_lpf = 1.0f;
    } else {
        // S-curve compression
        out_mag_sq_lpf = out_mag_sq_lpf * (1.5f - ((out_mag_sq_lpf * out_mag_sq_lpf) / 2.0f));
    }
}

} /* namespace dsp */
