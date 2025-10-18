#ifndef __LINEAR_RESAMPLER_H__
#define __LINEAR_RESAMPLER_H__

namespace dsp {
namespace interpolation {

    class LinearResampler {
      public:
        void configure(const float input_rate, const float output_rate) {
            phase_increment = calculate_increment(input_rate, output_rate);
        }

        template <typename InterpolatedSampleHandler> void operator()(const float sample, InterpolatedSampleHandler interpolated_sample_handler) {
            const float sample_delta = sample - last_sample;
            while (phase < 1.0f) {
                const float interpolated_value = last_sample + phase * sample_delta;
                interpolated_sample_handler(interpolated_value);
                phase += phase_increment;
            }
            last_sample = sample;
            phase -= 1.0f;
        }

        void advance(const float fraction) {
            phase += (fraction * phase_increment);
        }

      private:
        float last_sample{0.0f};
        float phase{0.0f};
        float phase_increment{0.0f};

        static constexpr float calculate_increment(const float input_rate, const float output_rate) {
            return input_rate / output_rate;
        }
    };

} /* namespace interpolation */
} /* namespace dsp */

#endif /*__LINEAR_RESAMPLER_H__*/
