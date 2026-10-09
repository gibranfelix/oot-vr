#pragma once

// The hand velocity at the release of a throw (the boomerang and the bomb). No game types. Tests:
// port/soh/tests/vr/VrBoomerangThrowTests.cpp and port/soh/tests/vr/VrBombThrowTests.cpp.

#include <cmath>
#include <cstdint>

namespace VrHandThrow {

struct Vec3 {
    float x, y, z;
};

// One hand sample: pos in game units, velocity in m/s.
struct Sample {
    Vec3 pos;
    Vec3 velMps;
    uint64_t timeNs;
};

// The throw velocity is the mean of the kAverageNs before the peak speed of the last kLookbackNs.
// The game reads the grip at 20 Hz, thus the release can come one tick after the peak.
constexpr uint64_t kLookbackNs = 100000000ull;
constexpr uint64_t kAverageNs = 50000000ull;

class History {
  public:
    void AddSample(const Sample& s) {
        mSamples[mNext] = s;
        mNext = (mNext + 1) % kCapacity;
        if (mCount < kCapacity) {
            mCount++;
        }
    }

    // Call when the hand is not tracked.
    void Clear() {
        mCount = 0;
    }

    // The velocity at the release, and the newest hand position. False when there are no samples.
    bool Release(Vec3* outVelMps, Vec3* outPos) const {
        if (mCount == 0) {
            return false;
        }
        const uint64_t newest = At(0).timeNs;
        int peak = 0;
        float peakSq = -1.0f;
        for (int age = 0; age < mCount && (newest - At(age).timeNs) <= kLookbackNs; age++) {
            const Vec3& v = At(age).velMps;
            const float sq = v.x * v.x + v.y * v.y + v.z * v.z;
            if (sq > peakSq) {
                peakSq = sq;
                peak = age;
            }
        }
        const uint64_t peakTime = At(peak).timeNs;
        Vec3 sum = { 0.0f, 0.0f, 0.0f };
        int n = 0;
        for (int age = peak; age < mCount && (peakTime - At(age).timeNs) <= kAverageNs; age++) {
            sum.x += At(age).velMps.x;
            sum.y += At(age).velMps.y;
            sum.z += At(age).velMps.z;
            n++;
        }
        *outVelMps = { sum.x / n, sum.y / n, sum.z / n };
        *outPos = At(0).pos;
        return true;
    }

  private:
    static constexpr int kCapacity = 32;

    const Sample& At(int age) const {
        return mSamples[(mNext - 1 - age + kCapacity) % kCapacity];
    }

    Sample mSamples[kCapacity] = {};
    int mNext = 0;
    int mCount = 0;
};

inline float Length(const Vec3& v) {
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

} // namespace VrHandThrow
