#pragma once
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// A simple one-pole RC filter supporting Low-Pass and High-Pass modes.
class OnePoleFilter {
public:
    enum class Type {
        LowPass,
        HighPass
    };

    OnePoleFilter() : mType(Type::LowPass), mSampleRate(44100.0f), mCutoff(20000.0f), a0(1.0f), b1(0.0f), z1(0.0f) {}

    void Init(float sampleRate) {
        mSampleRate = sampleRate;
        CalculateCoefficients();
        Reset();
    }

    void SetType(Type type) {
        if (mType != type) {
            mType = type;
            CalculateCoefficients();
        }
    }

    void SetCutoff(float cutoffFreq) {
        if (mCutoff != cutoffFreq) {
            mCutoff = cutoffFreq;
            CalculateCoefficients();
        }
    }

    inline float Process(float input) {
        z1 = input * a0 + z1 * b1;
        
        if (mType == Type::LowPass) {
            return z1;
        } else { // HighPass
            return input - z1;
        }
    }

    void Reset() {
        z1 = 0.0f;
    }

private:
    void CalculateCoefficients() {
        // Simple one-pole IIR coefficient calculation
        float w = 2.0f * M_PI * mCutoff / mSampleRate;
        // Pre-warp the cutoff frequency for bilinear transform
        float b = 2.0f - std::cos(w);
        b1 = b - std::sqrt(b * b - 1.0f);
        a0 = 1.0f - b1;
    }

    Type mType;
    float mSampleRate;
    float mCutoff;
    float a0;
    float b1;
    float z1; // state
};
