#pragma once

// The belt (VrBomb.cpp, VrNut.cpp): take the item with the grip, throw it with the arm. No game
// types. Tests: port/soh/tests/vr/VrBombThrowTests.cpp and port/soh/tests/vr/VrNutThrowTests.cpp.

#include "VrHandThrow.h"

#include <cmath>

namespace VrBeltThrow {

using Vec3 = VrHandThrow::Vec3;
using Sample = VrHandThrow::Sample;

// The belt is below the eyes of the player, in front of Link.
constexpr float kBeltBelowEyesM = 0.6f;
constexpr float kBeltForwardM = 0.2f;
// The hand takes the item at this distance from the belt.
constexpr float kGrabRadiusM = 0.22f;
// A grip that is pushed this number of ticks (0.3 s) before the hand gets to the belt takes the
// item.
constexpr int kGripGraceTicks = 6;
// Slower than kMinThrowSpeedMps: the item falls from the hand.
constexpr float kMinThrowSpeedMps = 1.5f;
// The throw does not use the world scale: child Link and adult Link throw the same distance.
constexpr float kThrowUnitsPerMeter = 40.0f;

// yawRad: the facing of Link. 0 is +z.
inline Vec3 BeltAnchor(const Vec3& eye, float yawRad, float unitsPerMeter) {
    const float fwd = kBeltForwardM * unitsPerMeter;
    return { eye.x + std::sin(yawRad) * fwd, eye.y - kBeltBelowEyesM * unitsPerMeter,
             eye.z + std::cos(yawRad) * fwd };
}

// The item center is offsetM out of the palm, thus the closed hand does not go into it.
// quat: the grip pose (x, y, z, w). +X of the OpenXR grip pose goes out of the left palm and into
// the right palm.
inline Vec3 PalmOffset(const float quat[4], bool leftHand, float unitsPerMeter, float offsetM) {
    const float d = (leftHand ? 1.0f : -1.0f) * offsetM * unitsPerMeter;
    const float x = quat[0];
    const float y = quat[1];
    const float z = quat[2];
    const float w = quat[3];
    // The local +X axis of the rotation.
    return { d * (1.0f - 2.0f * (y * y + z * z)), d * (2.0f * (x * y + w * z)), d * (2.0f * (x * z - w * y)) };
}

// The hand velocity in m/s to the item velocity in units for each tick. maxSpeed: units for each
// tick.
inline Vec3 ThrowVelocity(const Vec3& velMps, float ticksPerSecond, float gain, float maxSpeed) {
    const float k = kThrowUnitsPerMeter / ticksPerSecond * gain;
    Vec3 v = { velMps.x * k, velMps.y * k, velMps.z * k };
    const float len = VrHandThrow::Length(v);
    if (len > maxSpeed) {
        const float c = maxSpeed / len;
        v = { v.x * c, v.y * c, v.z * c };
    }
    return v;
}

struct HandInput {
    bool gripHeld;
    Vec3 pos; // Game units.
};

struct Input {
    bool beltReady; // An item is on the belt, and the hand can take it now.
    bool carrying;  // A hand holds the item.
    bool swapChord; // The two grips that draw the sword.
    HandInput hands[2];
    Vec3 beltPos;
    float grabRadius; // Game units.
    int firstHand;    // When the two hands take the item in the same tick, this hand gets it.
};

enum class Event { None, Grab, Throw, Drop };

struct Result {
    Event event = Event::None;
    int hand = -1;
    Vec3 pos = { 0.0f, 0.0f, 0.0f };    // Throw and Drop: where the item starts.
    Vec3 velMps = { 0.0f, 0.0f, 0.0f }; // Throw: the hand velocity.
    bool inReach[2] = { false, false };    // The hand is at the belt.
    bool reachPulse[2] = { false, false }; // The hand came to the belt in this tick.
};

class Holder {
  public:
    void AddSample(int hand, const Sample& s) {
        mHistory[hand].AddSample(s);
    }

    // Call when the hand is not tracked.
    void Clear(int hand) {
        mHistory[hand].Clear();
    }

    // The hand that holds the item, or -1.
    int Hand() const {
        return mHand;
    }

    // Call one time in each game tick, after AddSample.
    Result Update(const Input& in) {
        Result r;
        for (int hand = 0; hand < 2; hand++) {
            if (!in.hands[hand].gripHeld) {
                mGripAge[hand] = kNotHeld;
            } else if (mGripAge[hand] == kNotHeld) {
                mGripAge[hand] = 0;
            } else if (mGripAge[hand] <= kGripGraceTicks) {
                mGripAge[hand]++;
            }
        }
        if (!in.carrying) {
            // The item is gone (a bomb exploded), or the game did not take the grab.
            mHand = -1;
        }
        if (mHand >= 0) {
            const HandInput& h = in.hands[mHand];
            if (in.swapChord) {
                r = Drop(h.pos);
            } else if (!h.gripHeld) {
                r = Release(h.pos);
            }
        } else if (in.beltReady && !in.carrying) {
            for (int i = 0; i < 2; i++) {
                const int hand = (i == 0) ? in.firstHand : (in.firstHand ^ 1);
                const HandInput& h = in.hands[hand];
                const bool reach = InReach(h.pos, in.beltPos, in.grabRadius);
                r.reachPulse[hand] = reach && !mInReach[hand];
                if (reach && (mGripAge[hand] != kNotHeld) && (mGripAge[hand] <= kGripGraceTicks) &&
                    (r.event == Event::None)) {
                    r.event = Event::Grab;
                    r.hand = hand;
                    mHand = hand;
                    // One push takes one item.
                    mGripAge[hand] = kGripGraceTicks + 1;
                }
            }
        }
        for (int hand = 0; hand < 2; hand++) {
            mInReach[hand] = in.beltReady && !in.carrying && InReach(in.hands[hand].pos, in.beltPos, in.grabRadius);
            r.inReach[hand] = mInReach[hand];
        }
        return r;
    }

  private:
    static bool InReach(const Vec3& hand, const Vec3& belt, float radius) {
        const Vec3 d = { hand.x - belt.x, hand.y - belt.y, hand.z - belt.z };
        return VrHandThrow::Length(d) <= radius;
    }

    Result Drop(const Vec3& handPos) {
        Result r;
        r.event = Event::Drop;
        r.hand = mHand;
        r.pos = handPos;
        mHand = -1;
        return r;
    }

    Result Release(const Vec3& handPos) {
        Vec3 v;
        Vec3 pos;
        if (!mHistory[mHand].Release(&v, &pos) || (VrHandThrow::Length(v) < kMinThrowSpeedMps)) {
            return Drop(handPos);
        }
        Result r;
        r.event = Event::Throw;
        r.hand = mHand;
        r.pos = pos;
        r.velMps = v;
        mHand = -1;
        return r;
    }

    VrHandThrow::History mHistory[2];
    int mHand = -1;
    // Ticks since the push of the grip. A grip that is held at the start is too old.
    static constexpr int kNotHeld = -1;
    int mGripAge[2] = { kGripGraceTicks + 1, kGripGraceTicks + 1 };
    bool mInReach[2] = { false, false };
};

} // namespace VrBeltThrow
