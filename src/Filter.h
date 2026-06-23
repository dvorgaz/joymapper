#pragma once
#include "Common.h"

class IFilter
{
public:
	virtual void Filter(const double* input, double* output, const STime& time) = 0;
};

class EWMAFilter : public IFilter
{
private:
    static constexpr double delta_RC = 1. / 60;
    static constexpr double noise_RC_max = 60.0;

    double kMinSmoothing;
    double kMaxSmoothing;
    double kSmoothingScaleCurve;

    double noise_RC = 0.0;
    double last_delta[6];
    double last_noise[6];
    double last_output[6];
    bool first_run = true;

public:
	EWMAFilter(double min, double max, double curve);
	void Filter(const double* input, double* output, const STime& time) override;
};