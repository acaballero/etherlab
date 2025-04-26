/*
 * FIR filter class, by Mike Perkins
 *
 * a simple C++ class for linear phase FIR filtering
 *
 * For background, see the post http://www.cardinalpeak.com/blog?p=1841
 *
 * Copyright (c) 2013, Cardinal Peak, LLC.  http://www.cardinalpeak.com
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1) Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * 3) Neither the name of Cardinal Peak nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * CARDINAL PEAK, LLC BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "firFilter.h"
#include "printf.h"
#include "window.h"
#include <algorithm>
#include <arm_math.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>

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

double kaiserBeta(double attenuation) {
    if (attenuation > 50.0) {
        return 0.1102 * (attenuation - 8.7);
    } else if (attenuation >= 21.0) {
        return 0.5842 * pow(attenuation - 21.0, 0.4) + 0.07886 * (attenuation - 21.0);
    } else {
        return 0.0;
    }
}

int estimateNumTaps(double attenuation, double transWidth, double fs) {
    return static_cast<int>(ceil((attenuation - 8.0) / (2.285 * 2 * PI * (transWidth / fs))));
}

double estimateAttenuation(int numTaps, double transitionWidth, double fs) {
    double deltaF = transitionWidth / fs;
    return 2.285 * (numTaps - 1) * 2 * PI * deltaF + 8.0;
}

bool designFIRKaiserLowpass(float *taps, double cutoffHz, double sampleRate, double transitionWidth = 0.0, int numTaps = 0, double rippleDb = -1.0,
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
        numTaps = estimateNumTaps(attenuationDb, transitionWidth, sampleRate);
        if (numTaps % 2 == 0) {
            numTaps++; // Ensure odd length
        }
    } else if (validateConstraints && transitionWidth > 0.0) {
        double estimatedA = estimateAttenuation(numTaps, transitionWidth, sampleRate);
        if (estimatedA < attenuationDb) {
            //    std::cerr << "⚠️ Warning: With " << numTaps << " taps and transition width of " << transitionWidth << " Hz, estimated attenuation is only
            //    "
            //        << estimatedA << " dB, below desired " << attenuationDb << " dB.\n";
            return false;
        }
    }

    double normCutoff = cutoffHz / sampleRate;
    double beta = kaiserBeta(attenuationDb);

    int M = numTaps - 1;

    printf("LPF Kaiser FIR: fx:%f,fs:%f,taps:%d\n", sampleRate, cutoffHz, numTaps);

    for (int n = 0; n < numTaps; ++n) {
        double x = n - M / 2.0;
        double win = besselI0(beta * sqrt(1 - pow(2.0 * x / M, 2))) / besselI0(beta);
        taps[n] = 2 * normCutoff * sinc(2 * normCutoff * x) * win;
        printf_(",%f", taps[n]);
    }
    printf_("\n");
    return true;
}

void designLPF(float *m_taps, int m_num_taps, float fs, float fx) {

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

void designHPF(float *m_taps, int m_num_taps, float Fs, float Fx) {

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

void designBPF(float *m_taps, int m_num_taps, float Fs, float Fx, float Fu) {
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

// Handles LPF and HPF case
bool generate_fir_filter_taps(filter_type filt_t, float32_t *m_taps, int m_num_taps, float fs, float fx, float fu) {

    if ((fs >= 0) && (fx >= 0 && fx <= fs / 2) && (m_num_taps >= 0 && m_num_taps <= MAX_FILTER_TAPS)) {

        bool b = true;

        if (filt_t == LPF) {
            b = designFIRKaiserLowpass(m_taps, fx, fs, 0, m_num_taps, 1, 40, true);
            // designLPF(m_taps, m_num_taps, fs, fx);
        } else if (filt_t == HPF) {
            designHPF(m_taps, m_num_taps, fs, fx);
        } else {
            designBPF(m_taps, m_num_taps, fs, fx, fu);
        }

        if (filt_t == BPF) {
            // Apply window (temporarily until BPF generation includes it

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
    // Taps must be reversed to use cmsis decimators
    std::reverse(m_taps, m_taps + m_num_taps);
    return b;
}
