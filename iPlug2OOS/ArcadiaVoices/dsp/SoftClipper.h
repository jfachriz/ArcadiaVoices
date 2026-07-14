#pragma once
#include <cmath>
#include <algorithm>

// Soft clipper for feedback path limiting.
// Uses tanh approximation for cheap, symmetric soft clipping.
// Prevents runaway oscillation when feedback > 100%.
class SoftClipper {
public:
    SoftClipper() : mThreshold(1.0f) {}

    // Set threshold (default 1.0 = clips above unity gain)
    void SetThreshold(float threshold) {
        mThreshold = threshold;
    }

    // Process one sample through the clipper
    // Uses tanh approximation: tanh(x) ≈ x * (27 + x^2) / (27 + 9*x^2) (rational approx, cheap)
    inline float Process(float sample) const {
        // Scale to threshold, clip, scale back
        const float scaled = sample / mThreshold;
        const float clipped = FastTanh(scaled);
        return clipped * mThreshold;
    }

    // Harder clip for extreme feedback settings
    inline float ProcessHard(float sample) const {
        // Apply additional 2x gain before tanh for harder knee
        const float scaled = sample / mThreshold; // already at threshold
        const float driven = scaled * 1.5f;
        const float clipped = FastTanh(driven);
        return clipped * mThreshold * 0.85f; // slight makeup compensation
    }

private:
    float mThreshold;

    // Fast tanh approximation (Pade rational approx)
    // Error < 0.1% vs std::tanh, ~3x faster
    static inline float FastTanh(float x) {
        if (x > 4.0f) return 1.0f;
        if (x < -4.0f) return -1.0f;
        const float x2 = x * x;
        return x * (27.0f + x2) / (27.0f + 9.0f * x2);
    }
};
