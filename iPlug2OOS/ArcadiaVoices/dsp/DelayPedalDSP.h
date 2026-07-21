#pragma once
#include "PitchDelayVoice.h"
#include "PitchDetector.h"
#include "TempoSync.h"
#include "PresetManager.h"
#include <cmath>
#include <cstring>
#include <algorithm>

// Top-level DSP orchestrator for Arcadia Voices.
// Owns:
//   - 2x PitchDelayVoice (left/right channels)
//   - 2x PitchDetector (one per channel, on dry input)
//   - 1x TempoSync (shared)
//   - 1x PresetManager (shared)
//   - Dry/wet mix, power/bypass routing
//
// Designed to be standalone-testable — no iPlug2 dependencies.
// Call ProcessBlock() from the audio thread.
class DelayPedalDSP {
public:
    DelayPedalDSP() : mSampleRate(44100.0f),
                      mMaxDelayMs(2000.0f),
                      mPowerOn(true),
                      mMixPct(50.0f),
                      mSyncMode(SyncMode::Free) {
        // One-time preset initialization (NOT in Init() — Init() is called
        // from OnReset() on transport/sample-rate changes and must NOT
        // reinitialize the preset manager, which would wipe user presets).
        mPresetManager.Init();
    }

    // Initialize with given sample rate. Must be called before ProcessBlock.
    void Init(float sampleRate) {
        mSampleRate = sampleRate;

        // Calculate buffer size per GUIDELINE Section 3.2:
        // rawSamples = ceil((maxDelayTimeMs / 1000.0) * maxSampleRateHz) + interpGuardSamples
        // = ceil(2.0 * 48000) + 4 = 96004 → next power of 2 = 131072
        const double maxSampleRate = 48000.0; // we support up to 48kHz
        const size_t rawSamples = static_cast<size_t>(
            std::ceil((mMaxDelayMs / 1000.0) * maxSampleRate)) + 4;
        size_t bufferSize = 1;
        while (bufferSize < rawSamples) bufferSize <<= 1;

        // Initialize left and right voices
        mVoice[0].Init(sampleRate, bufferSize);
        mVoice[1].Init(sampleRate, bufferSize);

        // Initialize pitch detectors
        mDetector[0].Init(sampleRate);
        mDetector[1].Init(sampleRate);

        // Initialize tempo sync
        // NOTE: PresetManager::Init() is NOT called here — it's a one-time
        // call from the constructor. Calling it from OnReset() via Init()
        // would wipe user-created presets on transport events.
        mTempoSync.Init();
    }

    // Process one audio block (stereo)
    // inputs/outputs are arrays of T* channels (2 channels for stereo)
    // nFrames = number of frames (samples per channel)
    // T is typically double (iPlug2 default) or float.
    template <typename T>
    void ProcessBlock(T** inputs, T** outputs, int nFrames) {
        // === BYPASS CHECK ===
        if (!mPowerOn) {
            // True bypass: dry signal passes through untouched.
            // Do not process delay/pitch/feedback at all.
            for (int c = 0; c < 2; c++) {
                if (outputs[c] != inputs[c]) {
                    std::memcpy(outputs[c], inputs[c], nFrames * sizeof(T));
                }
            }
            return;
        }

        // Process each frame (convert to float for DSP engine)
        for (int s = 0; s < nFrames; s++) {
            const float inL = static_cast<float>(inputs[0][s]);
            const float inR = static_cast<float>(inputs[1][s]);

            // Feed dry input to pitch detectors (update every ~43ms hop)
            mDetector[0].ProcessSample(inL);
            mDetector[1].ProcessSample(inR);

            // Run pitch detection at hop intervals
            if (mDetectCounter++ >= mDetector[0].HopSize()) {
                mDetectCounter = 0;
                mDetector[0].Detect();
                mDetector[1].Detect();
                
                // Adaptive Grain Size:
                // Period = 1.0 / frequency. We want grain size to be roughly 1-2 periods.
                // 30ms is a safe default for non-pitched audio.
                for (int ch = 0; ch < 2; ch++) {
                    float freq = mDetector[ch].DetectedFreq();
                    float confidence = mDetector[ch].Confidence();
                    float targetGrainMs = 30.0f; // Default
                    if (freq > 20.0f && confidence > 0.6f) { // If we have a confident pitch
                        float periodMs = (1.0f / freq) * 1000.0f;
                        // Use ~2 periods for grain size, bounded between 15ms and 60ms
                        targetGrainMs = std::max(15.0f, std::min(60.0f, periodMs * 2.0f));
                    }
                    mVoice[ch].UpdateTargetGrainSizeMs(targetGrainMs);
                }
            }

            // Process left and right channels independently
            const float wetL = mVoice[0].Process(inL);
            const float wetR = mVoice[1].Process(inR);

            // Dry/wet mix
            const float mix = mMixPct * 0.01f;
            const float dry = 1.0f - mix;
            outputs[0][s] = static_cast<T>(inL * dry + wetL * mix);
            outputs[1][s] = static_cast<T>(inR * dry + wetR * mix);
        }
    }

    // ---- Parameter setters (called from UI thread / iPlug2 params) ----

    // Power / bypass toggle
    void SetPowerOn(bool on) {
        mPowerOn = on;
        // Note: buffer is NOT cleared on bypass toggle (Section 3.2).
        // When power is turned back on, in-flight repeats resume.
        // Pitch detectors should not update when power is off (handled
        // by the early-out in ProcessBlock).
    }
    bool IsPowerOn() const { return mPowerOn; }

    // Delay time in ms (used when syncMode == Free)
    void SetDelayTimeMs(float ms) {
        mDelayTimeMs = ms;
        mVoice[0].SetDelayTimeMs(ms);
        mVoice[1].SetDelayTimeMs(ms);
    }
    float DelayTimeMs() const { return mDelayTimeMs; }

    // Feedback 0-110%
    void SetFeedbackPct(float pct) {
        mFeedbackPct = pct;
        mVoice[0].SetFeedbackPct(pct);
        mVoice[1].SetFeedbackPct(pct);
    }
    float FeedbackPct() const { return mFeedbackPct; }

    // Dry/wet mix 0-100%
    void SetMixPct(float pct) {
        mMixPct = pct;
    }
    float MixPct() const { return mMixPct; }

    // Modulation depth 0-100%
    void SetModDepth(float depth) {
        mModDepth = depth;
        mVoice[0].SetModDepth(depth);
        mVoice[1].SetModDepth(depth);
    }
    float ModDepth() const { return mModDepth; }

    // Left channel pitch shift in semitones (-24 to +24)
    void SetPitchLSemitones(int semitones) {
        mPitchLSemitones = semitones;
        mVoice[0].SetPitchSemitones(semitones);
    }
    int PitchLSemitones() const { return mPitchLSemitones; }

    // Right channel pitch shift in semitones (-24 to +24)
    void SetPitchRSemitones(int semitones) {
        mPitchRSemitones = semitones;
        mVoice[1].SetPitchSemitones(semitones);
    }
    int PitchRSemitones() const { return mPitchRSemitones; }

    // Sync mode
    void SetSyncMode(SyncMode mode) {
        mSyncMode = mode;
        if (mode == SyncMode::Free) {
            // Restore manual time value
            SetDelayTimeMs(mDelayTimeMs);
        } else {
            // Recalculate from tempo
            UpdateSyncedDelay();
        }
    }
    SyncMode CurrentSyncMode() const { return mSyncMode; }

    // Set host BPM for tempo sync
    void SetHostBPM(double bpm) {
        mTempoSync.SetBPM(bpm);
        if (mSyncMode == SyncMode::Synced) {
            UpdateSyncedDelay();
        }
    }

    // Set note division index (0-6)
    void SetNoteDivision(int index) {
        mTempoSync.SetDivision(index);
        if (mSyncMode == SyncMode::Synced) {
            UpdateSyncedDelay();
        }
    }
    int NoteDivisionIndex() const { return mTempoSync.DivisionIndex(); }
    const char* NoteDivisionLabel() const { return mTempoSync.DivisionLabel(); }

    // TempoSync reference (for UI reads)
    const TempoSync& GetTempoSync() const { return mTempoSync; }

    // Pitch detector access (for UI reads of live-detected pitch)
    const PitchDetector& Detector(int channel) const {
        return mDetector[channel & 1];
    }

    // Preset manager (for preset load/save)
    PresetManager& GetPresetManager() { return mPresetManager; }
    const PresetManager& GetPresetManager() const { return mPresetManager; }

    // Apply a preset to all parameters
    void ApplyPreset(const PresetSnapshot& preset) {
        SetPowerOn(preset.powerOn);
        SetDelayTimeMs(preset.timeMs);
        SetFeedbackPct(preset.feedbackPct);
        SetMixPct(preset.mixPct);
        SetModDepth(preset.modPct);
        SetPitchLSemitones(static_cast<int>(preset.pitchL));
        SetPitchRSemitones(static_cast<int>(preset.pitchR));
        if (preset.syncMode) {
            SetSyncMode(SyncMode::Synced);
            SetNoteDivision(preset.noteDivision);
        } else {
            SetSyncMode(SyncMode::Free);
        }
    }

    // Clear delay buffers (for full reset)
    void Reset() {
        mVoice[0].Reset();
        mVoice[1].Reset();
        mDetector[0].Reset();
        mDetector[1].Reset();
        mDetectCounter = 0;
    }

    // Get sample rate
    float SampleRate() const { return mSampleRate; }

private:
    float mSampleRate;
    float mMaxDelayMs;
    bool mPowerOn;
    float mMixPct;
    float mDelayTimeMs;
    float mFeedbackPct;
    float mModDepth;
    int mPitchLSemitones = 0;
    int mPitchRSemitones = 0;
    SyncMode mSyncMode;

    PitchDelayVoice mVoice[2];
    PitchDetector mDetector[2];
    TempoSync mTempoSync;
    PresetManager mPresetManager;

    int mDetectCounter = 0;

    // Update delay from tempo sync
    void UpdateSyncedDelay() {
        const double delayMs = mTempoSync.CalcDelayMs();
        mDelayTimeMs = static_cast<float>(delayMs);
        for (int c = 0; c < 2; c++) {
            mVoice[c].SetDelayTimeMs(static_cast<float>(delayMs));
        }
    }
};
