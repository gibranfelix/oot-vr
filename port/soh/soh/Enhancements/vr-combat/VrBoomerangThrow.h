#pragma once

// The boomerang grip throw (VrBoomerang.cpp). No game types. Tests:
// port/soh/tests/vr/VrBoomerangThrowTests.cpp.

#include "VrBoomerangFlight.h"

#include <cmath>
#include <cstdint>

namespace VrBoomerangThrow {

struct Vec3 {
    float x, y, z;
};

// One hand sample: pos in game units, velocity in m/s.
struct Sample {
    Vec3 pos;
    Vec3 velMps;
    uint64_t timeNs;
};

// Slower than kMinThrowSpeedMps: no throw. The range goes from kMinRange to kMaxRange.
constexpr float kMinThrowSpeedMps = 2.0f;
constexpr float kFullThrowSpeedMps = 6.0f;
constexpr float kMinRange = 180.0f;
constexpr float kMaxRange = 600.0f;
// A throw to the back of Link does not go.
constexpr float kMinForwardDot = 0.0f;
// The throw velocity is the mean of the kAverageNs before the peak speed of the last kLookbackNs.
// The game reads the grip at 20 Hz, thus the release can come one tick after the peak.
constexpr uint64_t kLookbackNs = 100000000ull;
constexpr uint64_t kAverageNs = 50000000ull;

inline float RangeForSpeed(float speedMps) {
    const float t = (speedMps - kMinThrowSpeedMps) / (kFullThrowSpeedMps - kMinThrowSpeedMps);
    const float c = (t < 0.0f) ? 0.0f : ((t > 1.0f) ? 1.0f : t);
    return kMinRange + (kMaxRange - kMinRange) * c;
}

// EnBoom.returnTimer for a VR flight of rangeUnits. stepScale: R_UPDATE_RATE * 0.5.
inline int MovesForRange(float rangeUnits, float stepScale) {
    const int moves = (int)std::ceil(rangeUnits / (VR_BOOMERANG_SPEED * stepScale) - 0.001f);
    return (moves < 1) ? 1 : moves;
}

// With Z-targeting, the boomerang turns to the target only in a cone of 30 degrees. EnBoom_Fly
// alone turns to a target up to 90 degrees to the side.
constexpr float kTargetConeCos = 0.866f;
inline bool TowardTarget(const Vec3& pos, const Vec3& dir, const Vec3& target) {
    const Vec3 d = { target.x - pos.x, target.y - pos.y, target.z - pos.z };
    const float len = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
    if (len < 1.0f) {
        return true;
    }
    return (d.x * dir.x + d.y * dir.y + d.z * dir.z) >= kTargetConeCos * len;
}

struct Input {
    bool canThrow;      // The boomerang is in the hand.
    bool gripHeld;      // The grip of the boomerang hand.
    bool otherGripHeld; // With gripHeld: the swap to the sword and the shield.
    Vec3 forward;       // The facing of Link, unit.
};

struct Throw {
    Vec3 pos;
    Vec3 dir; // Unit.
    float range;
};

class Detector {
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

    // Call one time in each game tick, after AddSample. Returns true on the tick of the throw.
    bool Update(const Input& in, Throw* out) {
        if (!in.canThrow || in.otherGripHeld) {
            mArmed = false;
            // After a cancel, the grip must be released before the next throw.
            mBlocked = in.gripHeld;
            return false;
        }
        if (in.gripHeld) {
            mArmed = !mBlocked;
            return false;
        }
        mBlocked = false;
        if (!mArmed) {
            return false;
        }
        mArmed = false;
        return Release(in, out);
    }

  private:
    static constexpr int kCapacity = 32;

    const Sample& At(int age) const {
        return mSamples[(mNext - 1 - age + kCapacity) % kCapacity];
    }

    bool Release(const Input& in, Throw* out) const {
        if (mCount == 0) {
            return false;
        }
        const uint64_t newest = At(0).timeNs;
        int peak = -1;
        float peakSq = 0.0f;
        for (int age = 0; age < mCount && (newest - At(age).timeNs) <= kLookbackNs; age++) {
            const Vec3& v = At(age).velMps;
            const float sq = v.x * v.x + v.y * v.y + v.z * v.z;
            if (sq > peakSq) {
                peakSq = sq;
                peak = age;
            }
        }
        if (peak < 0) {
            return false;
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
        const Vec3 v = { sum.x / n, sum.y / n, sum.z / n };
        const float speed = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
        if (speed < kMinThrowSpeedMps) {
            return false;
        }
        const Vec3 dir = { v.x / speed, v.y / speed, v.z / speed };
        if (dir.x * in.forward.x + dir.y * in.forward.y + dir.z * in.forward.z < kMinForwardDot) {
            return false;
        }
        out->pos = At(0).pos;
        out->dir = dir;
        out->range = RangeForSpeed(speed);
        return true;
    }

    Sample mSamples[kCapacity] = {};
    int mNext = 0;
    int mCount = 0;
    bool mArmed = false;
    bool mBlocked = false;
};

} // namespace VrBoomerangThrow
