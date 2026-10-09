#pragma once

// The boomerang grip throw (VrBoomerang.cpp). No game types. Tests:
// port/soh/tests/vr/VrBoomerangThrowTests.cpp.

#include "VrBoomerangFlight.h"
#include "VrHandThrow.h"

#include <cmath>

namespace VrBoomerangThrow {

using Vec3 = VrHandThrow::Vec3;
using Sample = VrHandThrow::Sample;

// Slower than kMinThrowSpeedMps: no throw. The range goes from kMinRange to kMaxRange.
constexpr float kMinThrowSpeedMps = 2.0f;
constexpr float kFullThrowSpeedMps = 6.0f;
constexpr float kMinRange = 180.0f;
constexpr float kMaxRange = 600.0f;
// A throw to the back of Link does not go.
constexpr float kMinForwardDot = 0.0f;

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
        mHistory.AddSample(s);
    }

    // Call when the hand is not tracked.
    void Clear() {
        mHistory.Clear();
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
    bool Release(const Input& in, Throw* out) const {
        Vec3 v;
        Vec3 pos;
        if (!mHistory.Release(&v, &pos)) {
            return false;
        }
        const float speed = VrHandThrow::Length(v);
        if (speed < kMinThrowSpeedMps) {
            return false;
        }
        const Vec3 dir = { v.x / speed, v.y / speed, v.z / speed };
        if (dir.x * in.forward.x + dir.y * in.forward.y + dir.z * in.forward.z < kMinForwardDot) {
            return false;
        }
        out->pos = pos;
        out->dir = dir;
        out->range = RangeForSpeed(speed);
        return true;
    }

    VrHandThrow::History mHistory;
    bool mArmed = false;
    bool mBlocked = false;
};

} // namespace VrBoomerangThrow
