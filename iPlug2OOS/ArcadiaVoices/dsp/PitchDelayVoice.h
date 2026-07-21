#pragma once
#include "CircularBuffer.h"
#include "PitchShifter.h"
#include "ModLFO.h"
#include "SoftClipper.h"
#include "ParamSmoother.h"
#include "Filter.h"

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
        
        mFeedbackLPF.Init(sampleRate);
        mFeedbackLPF.SetType(OnePoleFilter::Type::LowPass);
        mFeedbackLPF.SetCutoff(3000.0f); // Analog delay style roll-off

        mFeedbackHPF.Init(sampleRate);
        mFeedbackHPF.SetType(OnePoleFilter::Type::HighPass);
        mFeedbackHPF.SetCutoff(200.0f); // Tame muddy low-end buildup

        mAntiAliasLPF.Init(sampleRate);
        mAntiAliasLPF.SetType(OnePoleFilter::Type::LowPass);
        mAntiAliasLPF.SetCutoff(20000.0f); // Default to Nyquist/passthrough

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

        // Adjust anti-alias cutoff if pitching up to prevent aliasing
        if (semitones > 0) {
            float ratio = std::pow(2.0f, semitones / 12.0f);
            float maxFreq = (mSampleRate * 0.5f) / ratio; // Nyquist divided by pitch ratio
            mAntiAliasLPF.SetCutoff(maxFreq);
        } else {
            mAntiAliasLPF.SetCutoff(20000.0f); // No anti-aliasing needed for pitch down/neutral
        }
    }

    // Update target grain size for adaptive pitching
    void UpdateTargetGrainSizeMs(float ms) {
        mPitchShifter.UpdateTargetGrainSizeMs(ms);
    }

    // Set sample rate (re-calculates delay line sizes)
    void SetSampleRate(float sampleRate) {
        mSampleRate = sampleRate;
        mPitchShifter.Init(sampleRate);
        mLFO.Init(sampleRate);
        mTimeSmoother.Init(sampleRate, 30.0f);
        mFeedbackLPF.Init(sampleRate);
        mFeedbackHPF.Init(sampleRate);
        mAntiAliasLPF.Init(sampleRate);
        // Restore cutoffs after Init resets them
        mFeedbackLPF.SetCutoff(3000.0f);
        mFeedbackHPF.SetCutoff(200.0f);
        SetPitchSemitones(static_cast<int>(mPitchSemitones)); // Re-calculate AA cutoff
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

        // Pre-process write sample: if pitching up, apply AA filter to prevent aliasing
        // in the pitch shifter. This applies to both input and feedback.
        float writeSample = input + mFeedback;
        if (mPitchSemitones > 0.0f) {
            writeSample = mAntiAliasLPF.Process(writeSample);
        }

        // Write feedback + input to delay buffer
        mDelayBuffer.Write(writeSample);

        // Read pitch-shifted signal from delay line
        const float pitchShifted = mPitchShifter.Process(mDelayBuffer, clampedDelay + modOffset);

        // Calculate feedback value (with clipping for >100% protection)
        // Clamp the pitch-shifted output to [-2, 2] before the feedback path
        const float feedbackGain = mFeedbackPct * 0.01f;
        const float clampedOutput = std::max(-2.0f, std::min(2.0f, pitchShifted));
        
        // Apply feedback filtering to emulate analog delay roll-off and reduce mud
        float filteredFeedback = mFeedbackHPF.Process(clampedOutput);
        filteredFeedback = mFeedbackLPF.Process(filteredFeedback);

        mFeedback = mClipper.Process(filteredFeedback * feedbackGain);

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
        mFeedbackLPF.Reset();
        mFeedbackHPF.Reset();
        mAntiAliasLPF.Reset();
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
    OnePoleFilter mFeedbackLPF;
    OnePoleFilter mFeedbackHPF;
    OnePoleFilter mAntiAliasLPF;

    float mFeedback;   // feedback sample (one-sample delay around the loop)
    float mLastOutput;
};
