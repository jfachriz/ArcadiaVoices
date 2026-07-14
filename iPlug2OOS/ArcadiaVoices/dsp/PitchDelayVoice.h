#pragma once
#include "CircularBuffer.h"
#include "PitchShifter.h"
#include "ModLFO.h"
#include "SoftClipper.h"
#include "ParamSmoother.h"

// Per-channel delay voice: owns the delay buffer, pitch shifter,
// LFO modulation, and feedback loop for one audio channel.
class PitchDelayVoice {
public:
    PitchDelayVoice() : mSampleRate(44100.0f),
                        mDelayTimeMs(350.0f),
                        mDelaySamples(0.0f),
                        mFeedbackPct(35.0f),
                        mModDepth(20.0f),
                        mPitchSemitones(0.0f) {}

    void Init(float sampleRate, size_t bufferSize) {
        mSampleRate = sampleRate;
        mDelayBuffer.Init(bufferSize);
        mPitchShifter.Init(sampleRate);
        mLFO.Init(sampleRate);
        mLFO.SetRate(0.3f);    // ~0.3 Hz default wobble
        mLFO.SetWaveform(0);   // sine
        mClipper.SetThreshold(1.0f);
        mTimeSmoother.Init(sampleRate, 30.0f); // 30ms smoothing for time changes
        mFeedback = 0.0f;
        mLastOutput = 0.0f;
    }

    // Set delay time in milliseconds
    void SetDelayTimeMs(float ms) {
        mDelayTimeMs = ms;
        const float targetSamples = (ms / 1000.0f) * mSampleRate;
        mTimeSmoother.SetTarget(targetSamples);
    }

    // Set raw delay in samples (used by tempo sync)
    void SetDelaySamples(float samples) {
        mTimeSmoother.SetTarget(samples);
    }

    // Set feedback percentage (0-110)
    void SetFeedbackPct(float pct) {
        mFeedbackPct = std::max(0.0f, std::min(110.0f, pct));
    }

    // Set modulation depth (0-100)
    void SetModDepth(float depth) {
        mModDepth = depth * 0.01f; // Normalize to 0-1
    }

    // Set pitch shift in semitones (-24 to +24)
    void SetPitchSemitones(int semitones) {
        mPitchSemitones = static_cast<float>(semitones);
        mPitchShifter.SetSemitones(semitones);
    }

    // Set sample rate (re-calculates delay line sizes)
    void SetSampleRate(float sampleRate) {
        mSampleRate = sampleRate;
        mPitchShifter.Init(sampleRate);
        mLFO.Init(sampleRate);
        mTimeSmoother.Init(sampleRate, 30.0f);
    }

    // Process one sample: dry input → delay → pitch shift → feedback → output
    inline float Process(float input) {
        // Get smoothed delay time (handles time changes without clicks)
        mDelaySamples = mTimeSmoother.Process();

        // Clamp delay to buffer capacity
        const float maxDelay = static_cast<float>(mDelayBuffer.MaxDelay());
        const float clampedDelay = std::min(mDelaySamples, maxDelay);

        // Get LFO modulation offset (applied to delay read position)
        const float lfoOut = mLFO.Process();
        const float modOffset = lfoOut * mModDepth * 5.0f; // ±5 samples at full depth (subtle chorus)

        // Write feedback + input to delay buffer
        const float writeSample = input + mFeedback;
        mDelayBuffer.Write(writeSample);

        // Read pitch-shifted signal from delay line
        const float pitchShifted = mPitchShifter.Process(mDelayBuffer, clampedDelay + modOffset);

        // Calculate feedback value (with clipping for >100% protection)
        // Clamp the pitch-shifted output to [-2, 2] before the feedback path
        // to prevent cubic interpolation overshoots from compounding through
        // the loop — spline overshoots that are inaudible on a single pass
        // become audible crackle when re-interpolated across multiple repeats.
        const float feedbackGain = mFeedbackPct * 0.01f;
        const float clampedOutput = std::max(-2.0f, std::min(2.0f, pitchShifted));
        mFeedback = mClipper.Process(clampedOutput * feedbackGain);

        mLastOutput = pitchShifted;
        return pitchShifted;
    }

    // Get the wet (processed) output without the dry signal
    inline float GetWet() const { return mLastOutput; }

    // Clear delay buffer and reset state
    void Reset() {
        mDelayBuffer.Clear();
        mPitchShifter.Reset();
        mLFO.Reset();
        mFeedback = 0.0f;
        mLastOutput = 0.0f;
        mTimeSmoother.Snap();
    }

    // Access the underlying delay buffer (for serialization, etc.)
    const CircularBuffer& DelayBuffer() const { return mDelayBuffer; }
    CircularBuffer& DelayBuffer() { return mDelayBuffer; }

    // Current smoothed delay in samples
    float CurrentDelaySamples() const { return mDelaySamples; }

private:
    float mSampleRate;
    float mDelayTimeMs;
    float mDelaySamples;
    float mFeedbackPct;
    float mModDepth;
    float mPitchSemitones;

    CircularBuffer mDelayBuffer;
    PitchShifter mPitchShifter;
    ModLFO mLFO;
    SoftClipper mClipper;
    ParamSmoother mTimeSmoother;

    float mFeedback;   // feedback sample (one-sample delay around the loop)
    float mLastOutput;
};
