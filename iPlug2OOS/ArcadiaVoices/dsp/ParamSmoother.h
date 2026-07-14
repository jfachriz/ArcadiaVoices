#pragma once
#include <cmath>

// One-pole lowpass parameter smoother — prevents clicks from abrupt
// parameter changes, particularly important for delay time and pitch shifts.
class ParamSmoother {
public:
    ParamSmoother() : mCurrent(0.0f), mTarget(0.0f), mCoeff(0.999f) {}

    void Init(float sampleRate, float timeConstantMs = 50.0f) {
        // Convert time constant to one-pole coefficient
        // coeff = exp(-1 / (timeConstantSec * sampleRate))
        const float tcSec = timeConstantMs / 1000.0f;
        mCoeff = std::exp(-1.0f / (tcSec * sampleRate));
    }

    void SetTarget(float target) { mTarget = target; }
    float Target() const { return mTarget; }

    // Get next smoothed value
    inline float Process() {
        mCurrent = mCoeff * mCurrent + (1.0f - mCoeff) * mTarget;
        return mCurrent;
    }

    // Snap immediately to target
    void Snap() { mCurrent = mTarget; }
    float Current() const { return mCurrent; }

private:
    float mCurrent;
    float mTarget;
    float mCoeff;
};
