#pragma once
#include "Filter.h"
#include <algorithm>
#include <cmath>

EWMAFilter::EWMAFilter(double min, double max, double curve)
{
    kMinSmoothing = std::fmin(1, std::fmax(0.01, min));
    kMaxSmoothing = std::fmin(1, std::fmax(0.01, max));
    kSmoothingScaleCurve = std::fmin(5, std::fmax(0.1, curve));
}

void EWMAFilter::Filter(const double* input, double* output, const STime& time)
{
    static constexpr double full_turn = 360;
    static constexpr double half_turn = 180;
    // Start the timer and initialise filter state if it's not running.
    if (first_run)
    {
        first_run = false;
        noise_RC = 0.0;
        std::copy(input, input + 6, last_output);
        std::fill(last_delta, last_delta + 6, 0.0);
        std::fill(last_noise, last_noise + 6, 0.0);
        return;
    }
    // Get the time in seconds since last run and restart the timer.
    const double dt = time.deltaTime;

    // Calculate delta_alpha and noise_alpha from dt.
    noise_RC = std::min(noise_RC + dt, noise_RC_max);
    double delta_alpha = dt / (dt + delta_RC);
    double noise_alpha = dt / (dt + noise_RC);

    // scale curve .01->1 where 1.0 is sqrt(norm_noise).
    const double smoothing_scale_curve = kSmoothingScaleCurve;
    // min/max smoothing .01->1
    const double min_smoothing = kMinSmoothing;
    const double max_smoothing = std::fmax(min_smoothing, kMaxSmoothing);

    // Calculate the new camera position.
    for (int i = 0; i < 6; i++)
    {
        using std::pow;

        // Calculate the current and smoothed delta.
        double input_value = input[i];

        if (i == 4) // Pitch axis
        {
            input_value *= 180. / 135;
        }

        double delta = input_value - last_output[i];
        if (i >= 3 && fabs(delta) > half_turn)
        {
            delta -= copysign(full_turn, delta);
            input_value -= copysign(full_turn, input_value);
        }
        last_delta[i] += delta_alpha * (delta - last_delta[i]);
        // Calculate the current and smoothed noise variance.
        double noise = last_delta[i] * last_delta[i];
        last_noise[i] = noise_alpha * noise + (1.0 - noise_alpha) * last_noise[i];
        // Normalise the noise between 0->1 for 0->9 variances (0->3 stddevs).
        double norm_noise = last_noise[i] < 1e-10 ? 0 : std::fmin(noise / (9.0 * last_noise[i]), 1.0);
        // Calculate the smoothing 0.0->1.0 from the normalized noise.
        double smoothing = 1.0 - pow(norm_noise, smoothing_scale_curve);
        double RC = (min_smoothing + smoothing * (max_smoothing - min_smoothing));
        // Calculate the dynamic alpha.
        double alpha = dt / (dt + RC);
        // Calculate the new output position.
        last_output[i] += alpha * (input_value - last_output[i]);
        output[i] = last_output[i];

        if (i == 4) // Pitch axis
        {
            output[i] *= 135. / 180;
        }
    }
}
