#pragma once
#include "CircularBuffer.h"
#include <cmath>

// Pitch shifter using dual-read-pointer + equal-power crossfade technique.
//
// Two read windows into a shared CircularBuffer, offset by half a grain.
// The read offset from the write pointer changes at rate (1 - pitchRatio).
// At ratio=1.0 (no shift), the offset stays constant = pure delay.
// At ratio>1.0, the offset shrinks → read catches up to write → higher pitch.
// At ratio<1.0, the offset grows → read falls further behind → lower pitch.
//
// When the offset crosses a grain boundary, the crossfade swaps between
// the two windows to hide the discontinuity.
class PitchShifter {
public:
    PitchShifter() : mPitchRatio(1.0f), mSampleRate(44100.0f),
                     mGrainSize(0.0f), mHalfGrain(0.0f),
                     mOffset(0.0f) {}

    void Init(float sampleRate) {
        mSampleRate = sampleRate;
        SetGrainSizeMs(30.0f);
        mTargetGrainSize = mGrainSize;
        mOffset = 0.0f;
    }

    // Set pitch shift ratio: 1.0 = no shift, 2.0 = up octave, 0.5 = down octave
    void SetPitchRatio(float ratio) {
        mPitchRatio = ratio;
    }

    // Set pitch shift in semitones (-24 to +24)
    void SetSemitones(int semitones) {
        mPitchRatio = std::pow(2.0f, semitones / 12.0f);
    }

    // Set grain size in milliseconds (typical range 15-60ms)
    // Instantly sets the grain size
    void SetGrainSizeMs(float ms) {
        mGrainSize = (ms / 1000.0f) * mSampleRate;
        mTargetGrainSize = mGrainSize;
        mHalfGrain = mGrainSize * 0.5f;
    }

    // Smoothly update target grain size (for adaptive grain sizing)
    void UpdateTargetGrainSizeMs(float ms) {
        mTargetGrainSize = (ms / 1000.0f) * mSampleRate;
    }

    // Process one sample: reads from delayLine at (delayOffset + readOffset)
    // Window A reads from the base position; Window B reads half-grain ahead.
    // Returns the pitch-shifted sample.
    inline float Process(const CircularBuffer& delayLine, float delayOffset) {
        // Smoothly interpolate grain size towards target
        if (std::abs(mTargetGrainSize - mGrainSize) > 0.1f) {
            mGrainSize += (mTargetGrainSize - mGrainSize) * 0.01f; // 100-sample time constant
            mHalfGrain = mGrainSize * 0.5f;
        }

        // The extra offset from pitch shifting changes at rate (1 - ratio).
        // At ratio=1.0: offset stays constant (pure delay).
        const float totalOffset = delayOffset + mOffset;

        // Two overlapping windows offset by half a grain
        const float readA = delayLine.Read(totalOffset);
        const float readB = delayLine.Read(totalOffset + mHalfGrain);

        // Compute crossfade phase from the offset within the grain window
        float phaseInGrain = mOffset;
        // Wrap to [0, grainSize) for phase computation
        while (phaseInGrain >= mGrainSize) phaseInGrain -= mGrainSize;
        while (phaseInGrain < 0.0f) phaseInGrain += mGrainSize;
        const float crossfadePhase = phaseInGrain / mGrainSize; // [0, 1)

        // Raised-cosine equal-power crossfade
        const float cf = 0.5f * (1.0f - std::cos(2.0f * M_PI * crossfadePhase));
        const float gainA = std::sqrt(1.0f - cf);
        const float gainB = std::sqrt(cf);

        const float out = readA * gainA + readB * gainB;

        // Advance the pitch offset: changes at rate (1 - ratio)
        // At ratio=1.0: stays the same. At ratio=2.0: decreases by 1/sample (reads newer data → higher pitch)
        mOffset += (1.0f - mPitchRatio);

        // Keep offset in a reasonable range (±grainSize around 0).
        // When it crosses a grain boundary, the windows swap roles seamlessly
        // because the crossfade phase also wraps.
        if (mOffset >= mGrainSize) {
            mOffset -= mGrainSize;
        } else if (mOffset <= -mGrainSize) {
            mOffset += mGrainSize;
        }

        return out;
    }

    // Reset all state
    void Reset() {
        mOffset = 0.0f;
    }

    float CurrentRatio() const { return mPitchRatio; }

private:
    float mPitchRatio;
    float mSampleRate;
    float mGrainSize;
    float mTargetGrainSize;
    float mHalfGrain;
    float mOffset; // pitch offset from base delay, changes at (1 - ratio) per sample
};
