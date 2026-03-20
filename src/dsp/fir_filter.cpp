#include "fir_filter.h"
#include "printf.h"
#include "window.h"
#include <algorithm>
#include <arm_math.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include "MemoryFree.h"
#include "status.h"

static double sinc(const double x) {
    if (x == 0) {
        return 1;
    }

    return sin(M_PI * x) / (M_PI * x);
}

double besselI0(double x) {
    double sum = 1.0, term = 1.0, k = 1, y = (x * x) / 4.0;
    while (term > 1e-10 * sum) {
        term *= y / (k * k);
        sum += term;
        ++k;
    }
    return sum;
}

double kaiser_beta(double attenuation) {
    if (attenuation > 50.0) {
        return 0.1102 * (attenuation - 8.7);
    } else if (attenuation >= 21.0) {
        return 0.5842 * pow(attenuation - 21.0, 0.4) + 0.07886 * (attenuation - 21.0);
    } else {
        return 0.0;
    }
}

int estimate_n_taps(double attenuation, double transWidth, double fs) {
    return static_cast<int>(ceil((attenuation - 8.0) / (2.285 * 2 * PI * (transWidth / fs))));
}

double estimate_attenuation(int numTaps, double transitionWidth, double fs) {
    double deltaF = transitionWidth / fs;
    return 2.285 * (numTaps - 1) * 2 * PI * deltaF + 8.0;
}

bool design_fir_kaiser_lpf(float *taps, double cutoffHz, double sampleRate, double transitionWidth = 0.0, int numTaps = 0, double rippleDb = -1.0,
                           double attenuationDb = -1.0, bool validateConstraints = true) {
    if (numTaps <= 0 && transitionWidth <= 0.0) {
        //   std::cerr << "Error: You must specify either numTaps or transitionWidth.\n";
    }

    // Derive attenuation if only ripple is given
    if (attenuationDb <= 0.0 && rippleDb > 0.0) {
        attenuationDb = rippleDb + 20.0; // Rule of thumb
    }

    if (attenuationDb <= 0.0) {
        return false;
        //  std::cerr << "Error: You must specify stopband attenuation or ripple.\n";
    }

    if (numTaps <= 0) {
        numTaps = estimate_n_taps(attenuationDb, transitionWidth, sampleRate);
        if (numTaps % 2 == 0) {
            numTaps++; // Ensure odd length
        }
    } else if (validateConstraints && transitionWidth > 0.0) {
        double estimatedA = estimate_attenuation(numTaps, transitionWidth, sampleRate);
        if (estimatedA < attenuationDb) {
            //    std::cerr << "⚠️ Warning: With " << numTaps << " taps and transition width of " << transitionWidth << " Hz, estimated attenuation is only
            //    "
            //        << estimatedA << " dB, below desired " << attenuationDb << " dB.\n";
            return false;
        }
    }

    double normCutoff = cutoffHz / sampleRate;
    double beta = kaiser_beta(attenuationDb);

    int M = numTaps - 1;

    LOG("[LPF Kaiser FIR]: fx:%f,fs:%f,taps:%d\n", sampleRate, cutoffHz, numTaps);

    for (int n = 0; n < numTaps; ++n) {
        double x = n - M / 2.0;
        double win = besselI0(beta * sqrt(1 - pow(2.0 * x / M, 2))) / besselI0(beta);
        taps[n] = 2 * normCutoff * sinc(2 * normCutoff * x) * win;
        // printf_(",%f", taps[n]);
    }
    // printf_("\n\n");
    return true;
}

bool design_complex_bandpass(float *taps, int num_taps, double fs, float center_freq, float bandwidth) {

    float real_taps[num_taps];

    // First, we generate a lowpass prototype with a moderate transition widht
    double transition_width = 0; // bandwidth / 2.0;

    bool ok = design_fir_kaiser_lpf(real_taps, bandwidth, fs, transition_width, num_taps, 1.0, 40.0, true);

    if (!ok) {
        return false;
    }

    // Then we shift it to the desired centrer frequency and make it complex by modulating with e^{j2πf₀n/fs}
    for (int n = 0; n < num_taps; ++n) {
        float phase = 2 * M_PI * center_freq * (n - num_taps / 2) / fs;
        taps[2 * n + 0] = real_taps[n] * cos(phase); // real part
        taps[2 * n + 1] = real_taps[n] * sin(phase); // imag part
    }

    // printf_("BPF complex FIR: fs:%f,center:%f,bw:%f,taps:%d\n", fs, center_freq, bandwidth, num_taps);

    for (int n = 0; n < num_taps * 2; ++n) {
        //     printf_(",%f", taps[n]);
    }
    // printf_("\n");

    return true;
}

void design_lpf(float *m_taps, int m_num_taps, float fs, float fx) {

    int n;

    double f = fx / fs;

    // printf("LPF: fx:%f,fs:%f,taps:%d\n", fx, fs, m_num_taps);

    for (n = 0; n < m_num_taps; n++) {
        int nn = n - m_num_taps / 2;
        m_taps[n] = 2.0 * f * sinc(2.0 * f * (double)nn);
        // printf_(",%f", m_taps[n]);
    }

    // printf_("\n");
}

void design_hpf(float *m_taps, int m_num_taps, float Fs, float Fx) {

    int n;
    double mm;

    float m_lambda = M_PI * Fx / (Fs / 2);

    for (n = 0; n < m_num_taps; n++) {
        mm = n - (m_num_taps - 1.0) / 2.0;
        if (mm == 0.0) {
            m_taps[n] = 1.0 - m_lambda / M_PI;
        } else {
            m_taps[n] = -sin(mm * m_lambda) / (mm * M_PI);
        }
    }
}

void design_bpf(float *m_taps, int m_num_taps, float Fs, float Fx, float Fu) {
    int n;
    double mm;

    float m_lambda = M_PI * Fx / (Fs / 2);
    float m_phi = M_PI * Fu / (Fs / 2);

    for (n = 0; n < m_num_taps; n++) {

        mm = n - (m_num_taps - 1.0) / 2.0;

        if (mm == 0.0) {
            m_taps[n] = (m_phi - m_lambda) / M_PI;
        } else {
            m_taps[n] = (sin(mm * m_phi) - sin(mm * m_lambda)) / (mm * M_PI);
        }
    }

    return;
}

// For BPF case, it generates an interleaved (i0,q0,i1,q1...) complex set of taps
bool generate_fir_filter_taps(filter_type filt_t, float32_t *m_taps, int m_num_taps, float fs, float fx, float fu) {

    if ((fs >= 0) && (fx >= 0 && fx <= fs / 2) && (m_num_taps >= 0 && m_num_taps <= MAX_FILTER_TAPS)) {

        bool b = true;

        if (filt_t == LPF) {
            b = design_fir_kaiser_lpf(m_taps, fx, fs, 0, m_num_taps, 1, 40, true);
            // designLPF(m_taps, m_num_taps, fs, fx);
        } else if (filt_t == HPF) {
            design_hpf(m_taps, m_num_taps, fs, fx);
        } else {
            // m_taps length must be m_num_taps * 2
            b = design_complex_bandpass(m_taps, m_num_taps, fs, fx, fu);
        }

        if (filt_t == HPF) {
            // Apply window (remove when HPF incorporates it)

            float fir_filter_window[m_num_taps];
            generate_window(1, fir_filter_window, m_num_taps);
            arm_mult_f32(m_taps, fir_filter_window, m_taps, m_num_taps);
        }

        return b;
    } else {
        return false;
    }
}

bool generate_fir_filter_taps_q15(filter_type filt_t, q15_t *m_taps, int m_num_taps, float fs, float fx, float fu) {

    float f_taps[m_num_taps];
    bool b = generate_fir_filter_taps(filt_t, f_taps, m_num_taps, fs, fx, fu);
    arm_float_to_q15(f_taps, m_taps, m_num_taps);

    return b;
}

void generate_raisedcosine_taps(float32_t *taps, int num_taps, int samples_per_symbol, float beta) {
    int M = num_taps / 2;

    float dt = 1.0f / samples_per_symbol;
    for (int i = -M; i <= M; ++i) {
        float t = i * dt;
        float pi_t = M_PI * t;
        float denom = 1.0f - 4.0f * beta * beta * t * t;

        float sinc = (t == 0.0f) ? 1.0f : sinf(pi_t) / pi_t;
        float cos_term = cosf(M_PI * beta * t);
        float val = (fabs(denom) < 1e-6f) ? beta / sqrtf(2.0f) * ((1 + 2 / M_PI) * sinf(M_PI / (4.0f * beta)) + (1 - 2 / M_PI) * cosf(M_PI / (4.0f * beta)))
                                          : sinc * cos_term / denom;
        taps[i + M] = val;
    }

    // Normalize tap energy (L2 norm)
    float energy = 0.0f;
    for (int i = 0; i < num_taps; ++i) {
        energy += taps[i] * taps[i];
    }

    float scale = 1.0f / sqrtf(energy);
    for (int i = 0; i < num_taps; ++i) {
        taps[i] *= scale;
    }
}
