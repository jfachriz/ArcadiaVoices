#pragma once
#include "CircularBuffer.h"
#include <cmath>

// Pitch shifter using dual-pointer rotating grain technique.
//
// Uses two read pointers into a shared CircularBuffer.
// A master phase from 0 to 1 drives both pointers.
// Pointer 1 phase = phase
// Pointer 2 phase = phase + 0.5 (wrapped)
// Pointers only wrap when their sine-based envelope is exactly 0,
// ensuring a perfectly click-free pitch shift.
class PitchShifter {
public:
    PitchShifter() : mPitchRatio(1.0f), mSampleRate(44100.0f),
                     mGrainSize(0.0f), mTargetGrainSize(0.0f),
                     mPhase(0.0f) {}

    void Init(float sampleRate) {
        mSampleRate = sampleRate;
        UpdateAdaptiveGrainSize();
        mGrainSize = mTargetGrainSize; // Snap to target instantly
        mPhase = 0.0f;
    }

    // Set pitch shift ratio: 1.0 = no shift, 2.0 = up octave, 0.5 = down octave
    void SetPitchRatio(float ratio) {
        mPitchRatio = ratio;
        UpdateAdaptiveGrainSize();
    }

    // Set pitch shift in semitones (-24 to +24)
    void SetSemitones(int semitones) {
        mPitchRatio = std::pow(2.0f, semitones / 12.0f);
        UpdateAdaptiveGrainSize();
    }

    // Set grain size in milliseconds (instantly sets)
    void SetGrainSizeMs(float ms) {
        mGrainSize = (ms / 1000.0f) * mSampleRate;
        mTargetGrainSize = mGrainSize;
    }

    // Smoothly update target grain size
    void UpdateTargetGrainSizeMs(float ms) {
        mTargetGrainSize = (ms / 1000.0f) * mSampleRate;
    }

    // Process one sample
    inline float Process(const CircularBuffer& delayLine, float delayOffset) {
        // Smoothly interpolate grain size towards target
        if (std::abs(mTargetGrainSize - mGrainSize) > 0.1f) {
            mGrainSize += (mTargetGrainSize - mGrainSize) * 0.0001f; 
        }

        // Calculate phase delta
        float deltaPhase = (1.0f - mPitchRatio) / mGrainSize;
        
        mPhase += deltaPhase;
        
        // Wrap master phase to [0, 1)
        while (mPhase >= 1.0f) mPhase -= 1.0f;
        while (mPhase < 0.0f) mPhase += 1.0f;
        
        float p1 = mPhase;
        float p2 = mPhase + 0.5f;
        if (p2 >= 1.0f) p2 -= 1.0f;
        
        float offset1 = p1 * mGrainSize;
        float offset2 = p2 * mGrainSize;
        
        float read1 = delayLine.Read(delayOffset + offset1);
        float read2 = delayLine.Read(delayOffset + offset2);
        
        // Equal power crossfade envelopes using sine
        // sin^2(x) + cos^2(x) = 1
        float env1 = std::sin(p1 * 3.14159265f);
        float env2 = std::sin(p2 * 3.14159265f);
        
        return read1 * env1 + read2 * env2;
    }

    // Reset all state
    void Reset() {
        mPhase = 0.0f;
    }

    float CurrentRatio() const { return mPitchRatio; }

private:
    void UpdateAdaptiveGrainSize() {
        float ms = 30.0f; // Default for no pitch shift
        if (mPitchRatio < 1.0f) {
            // Pitching down: larger grains to preserve bass (up to 55ms)
            float factor = (1.0f - mPitchRatio) / 0.75f; // 0 to 1 for -24st
            if (factor > 1.0f) factor = 1.0f;
            ms = 30.0f + (factor * 25.0f);
        } else if (mPitchRatio > 1.0f) {
            // Pitching up: smaller grains for transients (down to 20ms)
            float factor = (mPitchRatio - 1.0f) / 3.0f; // 0 to 1 for +24st
            if (factor > 1.0f) factor = 1.0f; 
            ms = 30.0f - (factor * 10.0f);
        }
        
        mTargetGrainSize = (ms / 1000.0f) * mSampleRate;
    }

    float mPitchRatio;
    float mSampleRate;
    float mGrainSize;
    float mTargetGrainSize;
    float mPhase; 
};
