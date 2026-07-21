#pragma once
#include <cmath>

// Low-frequency oscillator for modulation of delay line read position.
// Produces sine or triangle wave at a given rate and depth.
class ModLFO {
public:
    ModLFO() : mPhase(0.0f), mSampleRate(44100.0f) {}

    void Init(float sampleRate) {
        mSampleRate = sampleRate;
        mPhase = 0.0f;
    }

    // Set LFO rate in Hz. Default ~0.3 Hz for chorus-like wobble.
    void SetRate(float rateHz) {
        mPhaseIncrement = rateHz / mSampleRate;
    }

    // Set waveform type: 0 = sine, 1 = triangle
    void SetWaveform(int type) {
        mWaveform = type;
    }

    // Returns modulation value in [-1.0, 1.0]
    inline float Process() {
        // Wrap phase to [0, 1)
        mPhase += mPhaseIncrement;
        if (mPhase >= 1.0f) mPhase -= 1.0f;

        float out;
        if (mWaveform == 0) {
            // Sine: sin(2pi * phase)
            out = std::sin(2.0f * 3.14159265f * mPhase);
        } else {
            // Triangle: 4*|phase - 0.5| - 1
            out = 4.0f * std::abs(mPhase - 0.5f) - 1.0f;
        }
        return out;
    }

    // Reset phase to 0
    void Reset() { mPhase = 0.0f; }

private:
    float mPhase;
    float mPhaseIncrement;
    float mSampleRate;
    int mWaveform = 0; // 0 = sine, 1 = triangle
};
