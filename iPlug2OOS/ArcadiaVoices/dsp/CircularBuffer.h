#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <algorithm>

// Ring buffer for delay lines. Power-of-2 size for fast bitmask wrapping.
// Allocated once at construction — no reallocation.
class CircularBuffer {
public:
    CircularBuffer() : mData(nullptr), mSize(0), mMask(0), mWriteIndex(0) {}

    ~CircularBuffer() { delete[] mData; }

    // Allocate buffer. capacity must be a power of 2.
    void Init(size_t capacity) {
        // Verify power-of-2 (or zero)
        if ((capacity & (capacity - 1)) != 0 && capacity != 0) {
            // Round up to next power of 2
            size_t p = 1;
            while (p < capacity) p <<= 1;
            capacity = p;
        }
        delete[] mData;
        mSize = capacity;
        mMask = capacity - 1;
        mData = new float[mSize]();
        mWriteIndex = 0;
    }

    // Write one sample at the current write position
    inline void Write(float sample) {
        mData[mWriteIndex & mMask] = sample;
        mWriteIndex++;
    }

    // Read sample at a fractional offset before the write position
    // delayOffset = how many samples back to read (float for interpolation)
    // Uses cubic interpolation for quality
    inline float Read(float delayOffset) const {
        const float readPos = static_cast<float>(mWriteIndex) - delayOffset - 1.0f;
        const int32_t idx = static_cast<int32_t>(std::floor(readPos));
        const float frac = readPos - std::floor(readPos);

        // 4-point cubic interpolation
        const float x0 = ReadAt(idx - 1);
        const float x1 = ReadAt(idx);
        const float x2 = ReadAt(idx + 1);
        const float x3 = ReadAt(idx + 2);

        return CubicInterp(x0, x1, x2, x3, frac);
    }

    // Linear interpolation read (cheaper, slightly lower quality)
    inline float ReadLinear(float delayOffset) const {
        const float readPos = static_cast<float>(mWriteIndex) - delayOffset - 1.0f;
        const int32_t idx = static_cast<int32_t>(std::floor(readPos));
        const float frac = readPos - std::floor(readPos);

        const float x0 = ReadAt(idx);
        const float x1 = ReadAt(idx + 1);

        return x0 + frac * (x1 - x0);
    }

    // Get the maximum usable delay in samples (size - guard samples)
    inline size_t MaxDelay() const { return mSize - 4; } // guard samples for cubic interp

    // Get capacity
    inline size_t Size() const { return mSize; }

    // Clear buffer (zeros out all samples)
    inline void Clear() {
        std::memset(mData, 0, mSize * sizeof(float));
        mWriteIndex = 0;
    }

    // Access raw buffer (for direct manipulation, e.g. feedback writes)
    inline float* Data() { return mData; }
    inline const float* Data() const { return mData; }
    inline size_t Mask() const { return mMask; }
    inline int32_t WriteIndex() const { return mWriteIndex; }

private:
    // Read sample at absolute index (with wrapping)
    inline float ReadAt(int32_t idx) const {
        return mData[idx & mMask];
    }

    // Cubic Hermite interpolation (Catmull-Rom)
    static inline float CubicInterp(float x0, float x1, float x2, float x3, float t) {
        const float t2 = t * t;
        const float t3 = t2 * t;
        return 0.5f * ((2.0f * x1) +
                       (-x0 + x2) * t +
                       (2.0f * x0 - 5.0f * x1 + 4.0f * x2 - x3) * t2 +
                       (-x0 + 3.0f * x1 - 3.0f * x2 + x3) * t3);
    }

    float* mData;
    size_t mSize;
    size_t mMask;
    int32_t mWriteIndex;
};
