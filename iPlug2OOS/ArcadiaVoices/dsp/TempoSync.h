#pragma once
#include <cmath>
#include <string>

// Sync mode: Free or Tempo-synced
enum class SyncMode { Free, Synced };

// Tempo sync module — converts host BPM + note division into delay milliseconds.
class TempoSync {
public:
    TempoSync() : mBPM(120.0), mCurrentDivision(1.0), mDivisionIndex(2) {
        Init();
    }

    void Init() {
        mDivisionLabels[0] = "1/32";   mDivisionValues[0] = 0.125;
        mDivisionLabels[1] = "1/16T";  mDivisionValues[1] = 0.16667;
        mDivisionLabels[2] = "1/16";   mDivisionValues[2] = 0.25;
        mDivisionLabels[3] = "1/8T";   mDivisionValues[3] = 0.33333;
        mDivisionLabels[4] = "1/8";    mDivisionValues[4] = 0.5;
        mDivisionLabels[5] = "1/8D";   mDivisionValues[5] = 0.75;
        mDivisionLabels[6] = "1/4T";   mDivisionValues[6] = 0.66667;
        mDivisionLabels[7] = "1/4";    mDivisionValues[7] = 1.0;
        mDivisionLabels[8] = "1/4D";   mDivisionValues[8] = 1.5;
        mDivisionLabels[9] = "1/2T";   mDivisionValues[9] = 1.33333;
        mDivisionLabels[10] = "1/2";   mDivisionValues[10] = 2.0;
        mDivisionLabels[11] = "1/2D";  mDivisionValues[11] = 3.0;
        mDivisionLabels[12] = "1 bar"; mDivisionValues[12] = 4.0;
        mDivisionLabels[13] = "2 bars"; mDivisionValues[13] = 8.0;
        mDivisionCount = 14;
        mDivisionIndex = 4; // Default: 1/8
        mCurrentDivision = mDivisionValues[mDivisionIndex];
    }

    // Set host BPM from transport info
    void SetBPM(double bpm) {
        mBPM = bpm;
    }

    // Get current BPM
    double BPM() const { return mBPM; }

    // Set note division by index (0-6)
    void SetDivision(int index) {
        if (index < 0) index = 0;
        if (index >= mDivisionCount) index = mDivisionCount - 1;
        mDivisionIndex = index;
        mCurrentDivision = mDivisionValues[index];
    }

    // Get current division index
    int DivisionIndex() const { return mDivisionIndex; }

    // Get current division label
    const char* DivisionLabel() const { return mDivisionLabels[mDivisionIndex]; }

    // Get current division multiplier
    double DivisionMultiplier() const { return mCurrentDivision; }

    // Number of available divisions
    int DivisionCount() const { return mDivisionCount; }

    // Get label for a specific division index
    const char* DivisionLabel(int index) const {
        if (index < 0 || index >= mDivisionCount) return "";
        return mDivisionLabels[index];
    }

    // Get value for a specific division index
    double DivisionValue(int index) const {
        if (index < 0 || index >= mDivisionCount) return 1.0;
        return mDivisionValues[index];
    }

    // Cycle to the next division (wraps around)
    void CycleForward() {
        mDivisionIndex = (mDivisionIndex + 1) % mDivisionCount;
        mCurrentDivision = mDivisionValues[mDivisionIndex];
    }

    // Cycle to the previous division
    void CycleBackward() {
        mDivisionIndex--;
        if (mDivisionIndex < 0) mDivisionIndex = mDivisionCount - 1;
        mCurrentDivision = mDivisionValues[mDivisionIndex];
    }

    // Convert BPM + division to delay time in milliseconds
    // delayTimeMs = (60000 / BPM) * divisionMultiplier
    // Assumes 4/4 time: 1 bar = 4 beats
    double CalcDelayMs() const {
        return (60000.0 / mBPM) * mCurrentDivision;
    }

    // Static helper: convert BPM + division value to ms
    static double CalcDelayMs(double bpm, double division) {
        return (60000.0 / bpm) * division;
    }

private:
    double mBPM;
    double mCurrentDivision;
    int mDivisionIndex;
    int mDivisionCount;

    // Note division labels and their beat multipliers (4/4 time)
    const char* mDivisionLabels[14];
    double mDivisionValues[14];
};
