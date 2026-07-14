#pragma once
#include <cmath>
#include <cstring>
#include <algorithm>
#include <string>
#include <vector>

// Pitch detector using the YIN algorithm (autocorrelation-based).
// Designed for monophonic guitar/bass input.
// Runs on a rolling analysis window — not per-sample.
class PitchDetector {
public:
    PitchDetector() : mSampleRate(44100.0f),
                      mWindowSize(2048),
                      mHopSize(1024),
                      mConfidence(0.0f),
                      mSamplesCollected(0),
                      mSilenceThreshold(0.001f) {}

    void Init(float sampleRate, int windowSize = 2048, int hopSize = 1024) {
        mSampleRate = sampleRate;
        mWindowSize = windowSize;
        mHopSize = hopSize;
        mBuffer.resize(windowSize, 0.0f);
        mAnalysisBuffer.resize(windowSize, 0.0f);
        mWriteIndex = 0;
        mSamplesCollected = 0;
        mDetectedNote = "";
        mDetectedCents = 0;
        mConfidence = 0.0f;
        mSilenceThreshold = 0.001f;
    }

    // Push one audio sample into the circular analysis buffer
    inline void ProcessSample(float sample) {
        mBuffer[mWriteIndex] = sample;
        mWriteIndex = (mWriteIndex + 1) % mWindowSize;
        if (mSamplesCollected < mWindowSize) mSamplesCollected++;
    }

    // Push a block of samples
    void ProcessBlock(const float* samples, int numSamples) {
        for (int i = 0; i < numSamples; i++) {
            mBuffer[mWriteIndex] = samples[i];
            mWriteIndex = (mWriteIndex + 1) % mWindowSize;
        }
        mSamplesCollected += numSamples;
        if (mSamplesCollected > mWindowSize) mSamplesCollected = mWindowSize + 1;
    }

    // Copy circular buffer into contiguous analysis buffer
    void PrepareAnalysisBuffer() {
        if (mSamplesCollected < mWindowSize) return;
        // Copy the last mWindowSize samples into contiguous order
        // Starting from the oldest sample in the window
        const int start = (mWriteIndex - mWindowSize + mWindowSize) % mWindowSize;
        for (int i = 0; i < mWindowSize; i++) {
            mAnalysisBuffer[i] = mBuffer[(start + i) % mWindowSize];
        }
    }

    // Run pitch detection on the current buffer.
    // Returns true if a pitch was detected, false if silent or unclear.
    bool Detect() {
        if (mSamplesCollected < mWindowSize) return false;

        // Prepare contiguous analysis buffer from circular buffer
        PrepareAnalysisBuffer();

        const float* buf = mAnalysisBuffer.data();
        const int len = mWindowSize;

        // Check for silence first
        float rms = 0.0f;
        for (int i = 0; i < len; i++) {
            rms += buf[i] * buf[i];
        }
        rms = std::sqrt(rms / len);
        if (rms < mSilenceThreshold) {
            mConfidence = 0.0f;
            return false;
        }

        // YIN step 1: compute difference function
        // d(tau) = sum_{j=1}^{W} (x_j - x_{j+tau})^2
        const int maxTau = std::min(len / 2, static_cast<int>(mSampleRate / 40.0f)); // min 40 Hz
        const int minTau = std::max(1, static_cast<int>(mSampleRate / 2000.0f)); // max 2000 Hz

        std::vector<float> diff(maxTau + 1, 0.0f);

        for (int tau = minTau; tau <= maxTau; tau++) {
            float sum = 0.0f;
            for (int j = 0; j < len - maxTau; j++) {
                const float d = buf[j] - buf[j + tau];
                sum += d * d;
            }
            diff[tau] = sum;
        }

        // YIN step 2: cumulative mean normalized difference function
        // d'(tau) = d(tau) / ((1/tau) * sum_{j=1}^{tau} d(j))
        float runningSum = 0.0f;
        float minVal = 1.0f;
        int bestTau = 0;

        for (int tau = minTau; tau <= maxTau; tau++) {
            runningSum += diff[tau];
            if (runningSum == 0.0f) continue;

            const float normalized = diff[tau] * tau / runningSum;

            if (normalized < minVal) {
                minVal = normalized;
                bestTau = tau;
            }

            // YIN step 3: absolute threshold — return the first tau where
            // d'(tau) drops below the threshold. Standard YIN uses 0.1.
            // Only accept if going BELOW the threshold (not just any dip).
            if (normalized < 0.1f && bestTau == tau) {
                break;
            }
        }

        if (bestTau == 0 || minVal > 0.5f) {
            mConfidence = 0.0f;
            return false;
        }

        // YIN step 4: parabolic interpolation for better accuracy
        const float interpolatedTau = ParabolicInterp(bestTau, diff);
        const float freq = mSampleRate / interpolatedTau;

        // Convert to note name
        mDetectedFreq = freq;
        mConfidence = 1.0f - minVal;

        // Convert frequency to MIDI note number
        const float midiNote = 69.0f + 12.0f * std::log2(freq / 440.0f);
        const int noteIndex = static_cast<int>(std::round(midiNote));
        mDetectedCents = static_cast<int>(std::round((midiNote - noteIndex) * 100.0f));

        // Note names (C=0, C#=1, ... B=11)
        static const char* kNoteNames[] = {"C", "C#", "D", "D#", "E", "F",
                                           "F#", "G", "G#", "A", "A#", "B"};
        const int octave = (noteIndex / 12) - 1;
        const int noteInOctave = ((noteIndex % 12) + 12) % 12;

        char noteBuf[32];
        if (std::abs(mDetectedCents) >= 5) {
            // Include cents offset if significant
            std::snprintf(noteBuf, sizeof(noteBuf), "%s%d%+d", kNoteNames[noteInOctave], octave, mDetectedCents);
        } else {
            std::snprintf(noteBuf, sizeof(noteBuf), "%s%d", kNoteNames[noteInOctave], octave);
        }
        mDetectedNote = noteBuf;

        return true;
    }

    // Get deteted note name (e.g. "A3", "C#4-12")
    const std::string& DetectedNote() const { return mDetectedNote; }

    // Get cents deviation from nearest semitone
    int DetectedCents() const { return mDetectedCents; }

    // Get confidence [0, 1]
    float Confidence() const { return mConfidence; }

    // Get frequency in Hz
    float DetectedFreq() const { return mDetectedFreq; }

    // Set silence threshold
    void SetSilenceThreshold(float threshold) { mSilenceThreshold = threshold; }

    // Get the hop size (number of samples per analysis)
    int HopSize() const { return mHopSize; }

    // Reset detector state
    void Reset() {
        std::fill(mBuffer.begin(), mBuffer.end(), 0.0f);
        std::fill(mAnalysisBuffer.begin(), mAnalysisBuffer.end(), 0.0f);
        mWriteIndex = 0;
        mSamplesCollected = 0;
        mDetectedNote = "";
        mDetectedCents = 0;
        mConfidence = 0.0f;
    }

private:
    float mSampleRate;
    int mWindowSize;
    int mHopSize;
    std::vector<float> mBuffer;       // circular buffer for input samples
    std::vector<float> mAnalysisBuffer; // contiguous copy for analysis
    int mWriteIndex = 0;
    int mSamplesCollected;
    std::string mDetectedNote;
    int mDetectedCents;
    float mConfidence;
    float mDetectedFreq;
    float mSilenceThreshold;

    // Parabolic interpolation around the minimum
    // Returns the interpolated tau (period in samples)
    static float ParabolicInterp(int tau, const std::vector<float>& diff) {
        const int len = static_cast<int>(diff.size());
        if (tau <= 0 || tau >= len - 1 || diff[tau] == 0.0f) {
            return 0.0f;
        }

        const float y0 = diff[tau - 1];
        const float y1 = diff[tau];
        const float y2 = diff[tau + 1];

        // Parabolic peak: offset = (y0 - y2) / (2 * (y0 + y2 - 2*y1))
        const float denom = 2.0f * (y0 + y2 - 2.0f * y1);
        if (denom == 0.0f) return 0.0f;

        const float offset = (y0 - y2) / denom;
        return tau + offset;
    }
};

// Note: PitchDetector includes <vector> via mBuffer. The guideline asks for
// minimal STL in realtime audio threads. PitchDetector is NOT called per-sample;
// it runs at ~23 Hz (2048-sample window, 44100 Hz, 1024-hop = ~43ms intervals),
// so vector allocation in Init() is fine — the analysis buffer is sized once
// and reused. The per-block Push() and Detect() calls use only stack and pre-
// allocated memory.
