#pragma once

// The bomb on the belt (VrBomb.cpp): take it with the grip, throw it with the arm. No game types.
// Tests: port/soh/tests/vr/VrBombThrowTests.cpp.

#include "VrHandThrow.h"

#include <cmath>

namespace VrBombThrow {

using Vec3 = VrHandThrow::Vec3;
using Sample = VrHandThrow::Sample;

// The belt is below the eyes of the player, in front of Link.
constexpr float kBeltBelowEyesM = 0.6f;
constexpr float kBeltForwardM = 0.2f;
// The hand takes the bomb at this distance from the belt.
constexpr float kGrabRadiusM = 0.15f;
// Slower than kMinThrowSpeedMps: the bomb falls from the hand.
constexpr float kMinThrowSpeedMps = 1.5f;
// The throw does not use the world scale: child Link and adult Link throw the same distance.
constexpr float kThrowUnitsPerMeter = 40.0f;
// A medium throw (4 m/s) goes as far as the vanilla throw.
constexpr float kThrowGain = 1.8f;
// EnBom gravity, in units for each tick squared.
constexpr float kBombGravity = 1.2f;
// Units for each tick. The longest throw goes 2.5 times the vanilla distance (160 units).
constexpr float kMaxThrowSpeed = 21.9f;
// The bomb model is 14 units wide. In VR it is kBombDiameterM wide in the real world.
constexpr float kBombModelDiameter = 14.0f;
constexpr float kBombDiameterM = 0.15f;

// The draw scale of the bomb model. Never larger than the vanilla bomb.
inline float DrawScale(float unitsPerMeter) {
    const float k = kBombDiameterM * unitsPerMeter / kBombModelDiameter;
    return (k > 1.0f) ? 1.0f : k;
}

// yawRad: the facing of Link. 0 is +z.
inline Vec3 BeltAnchor(const Vec3& eye, float yawRad, float unitsPerMeter) {
    const float fwd = kBeltForwardM * unitsPerMeter;
    return { eye.x + std::sin(yawRad) * fwd, eye.y - kBeltBelowEyesM * unitsPerMeter,
             eye.z + std::cos(yawRad) * fwd };
}

// The hand velocity in m/s to the bomb velocity in units for each tick.
inline Vec3 ThrowVelocity(const Vec3& velMps, float ticksPerSecond) {
    const float k = kThrowUnitsPerMeter / ticksPerSecond * kThrowGain;
    Vec3 v = { velMps.x * k, velMps.y * k, velMps.z * k };
    const float len = VrHandThrow::Length(v);
    if (len > kMaxThrowSpeed) {
        const float c = kMaxThrowSpeed / len;
        v = { v.x * c, v.y * c, v.z * c };
    }
    return v;
}

struct HandInput {
    bool gripHeld;
    Vec3 pos; // Game units.
};

struct Input {
    bool beltReady; // A bomb is on the belt, and the hand can take it now.
    bool carrying;  // Link carries a bomb.
    bool swapChord; // The two grips that draw the sword.
    HandInput hands[2];
    Vec3 beltPos;
    float grabRadius; // Game units.
    int firstHand;    // When the two hands take the bomb in the same tick, this hand gets it.
};

enum class Event { None, Grab, Throw, Drop };

struct Result {
    Event event = Event::None;
    int hand = -1;
    Vec3 pos = { 0.0f, 0.0f, 0.0f };    // Throw and Drop: where the bomb starts.
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

    // The hand that holds the bomb, or -1.
    int Hand() const {
        return mHand;
    }

    // Call one time in each game tick, after AddSample.
    Result Update(const Input& in) {
        Result r;
        if (!in.carrying) {
            // The bomb exploded, or the game did not take the grab.
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
                if (reach && h.gripHeld && !mGripPrev[hand] && (r.event == Event::None)) {
                    r.event = Event::Grab;
                    r.hand = hand;
                    mHand = hand;
                }
            }
        }
        for (int hand = 0; hand < 2; hand++) {
            mGripPrev[hand] = in.hands[hand].gripHeld;
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
    // A grip that is held at the start must be released before it takes the bomb.
    bool mGripPrev[2] = { true, true };
    bool mInReach[2] = { false, false };
};

} // namespace VrBombThrow
