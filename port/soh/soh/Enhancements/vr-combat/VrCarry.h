#pragma once

// The carry (VrCarry.cpp, #158): the hand lifts an object and throws it. No game types.
// Tests: port/soh/tests/vr/VrCarryTests.cpp.

#include "VrBeltThrow.h"

#include <cmath>

namespace VrCarry {

using Vec3 = VrHandThrow::Vec3;
using Sample = VrHandThrow::Sample;

// The objects that the hand lifts. The game lifts them with the A button.
enum class Kind { None, Pot, SmallRock, Bush, Crate, BombFlower, Bomb, Cucco };

// The collider cylinder of the object and its gravity, in game units. The cylinder starts yShift
// above the actor position.
struct Shape {
    float radius;
    float height;
    float yShift;
    float gravity; // Units for each tick squared, positive.
};

// The values come from the collider and the gravity of each actor.
inline Shape ShapeOf(Kind kind) {
    switch (kind) {
        case Kind::Pot:
            return { 9.0f, 26.0f, 0.0f, 1.2f }; // ObjTsubo
        case Kind::SmallRock:
            return { 10.0f, 18.0f, -2.0f, 1.2f }; // EnIshi
        case Kind::Bush:
            return { 12.0f, 44.0f, 0.0f, 3.2f }; // EnKusa
        case Kind::Crate:
            return { 12.0f, 27.0f, 0.0f, 1.2f }; // ObjKibako
        case Kind::BombFlower:
            return { 9.0f, 18.0f, 0.0f, 1.5f }; // EnBombf
        case Kind::Bomb:
            return { 7.0f, 14.0f, 0.0f, 1.2f }; // EnBom: the model, at the vanilla scale.
        case Kind::Cucco:
            return { 15.0f, 25.0f, 4.0f, 2.0f }; // EnNiw
        case Kind::None:
            break;
    }
    return { 0.0f, 0.0f, 0.0f, 0.0f };
}

// The hand lifts an object at this distance from its collider.
constexpr float kGrabMarginM = 0.1f;
// A grip that is pushed this number of ticks (0.3 s) before the hand gets to the object lifts it.
constexpr int kGripGraceTicks = VrBeltThrow::kGripGraceTicks;
// Slower than this, the object falls from the hand.
constexpr float kMinThrowSpeedMps = VrBeltThrow::kMinThrowSpeedMps;

// The distance from p to a vertical cylinder. 0 when p is in the cylinder.
inline float DistanceToCylinder(const Vec3& p, const Vec3& base, float radius, float height) {
    const float dx = p.x - base.x;
    const float dz = p.z - base.z;
    const float out = std::fmax(std::sqrt(dx * dx + dz * dz) - radius, 0.0f);
    float dy = 0.0f;
    if (p.y < base.y) {
        dy = base.y - p.y;
    } else if (p.y > base.y + height) {
        dy = p.y - (base.y + height);
    }
    return std::sqrt(out * out + dy * dy);
}

// An object that the game lets Link lift now.
struct Candidate {
    Vec3 pos; // The actor position.
    Shape shape;
};

// The candidate nearest to the hand, in reach. -1 when no candidate is in reach. margin: game
// units.
inline int PickTarget(const Vec3& hand, const Candidate* candidates, int count, float margin) {
    int best = -1;
    float bestDist = 0.0f;
    for (int i = 0; i < count; i++) {
        const Candidate& c = candidates[i];
        const Vec3 base = { c.pos.x, c.pos.y + c.shape.yShift, c.pos.z };
        const float d = DistanceToCylinder(hand, base, c.shape.radius, c.shape.height);
        if ((d <= margin) && ((best < 0) || (d < bestDist))) {
            best = i;
            bestDist = d;
        }
    }
    return best;
}

// A throw with the same hand speed goes the same distance for all objects. The bomb belt sets the
// speed for the gravity of the bomb.
constexpr float kRefGravity = 1.2f;
constexpr float kThrowGain = 1.8f;
constexpr float kMaxThrowSpeed = 21.9f;

// The hand velocity in m/s to the object velocity in units for each tick.
inline Vec3 ThrowVelocity(const Vec3& velMps, float ticksPerSecond, float gravity) {
    const float k = std::sqrt(((gravity > 0.0f) ? gravity : kRefGravity) / kRefGravity);
    return VrBeltThrow::ThrowVelocity(velMps, ticksPerSecond, kThrowGain * k, kMaxThrowSpeed * k);
}

// A drop puts the object down when its bottom is this near to the floor. Higher, the object falls.
constexpr float kPutDownHeightM = 0.15f;
// The speed of a falling object, in units for each tick. At zero speed, a crate (ObjKibako) puts
// itself down in the air.
constexpr float kFallSpeed = 0.2f;

// The speed of a dropped object. heightAboveFloor: game units.
inline float DropSpeed(float heightAboveFloor, float unitsPerMeter) {
    return (heightAboveFloor > kPutDownHeightM * unitsPerMeter) ? kFallSpeed : 0.0f;
}

// Rotates v with the unit quaternion q (x, y, z, w).
inline Vec3 Rotate(const float q[4], const Vec3& v) {
    const float x = q[0];
    const float y = q[1];
    const float z = q[2];
    const float w = q[3];
    // t = 2 * cross(q.xyz, v); v' = v + w * t + cross(q.xyz, t)
    const Vec3 t = { 2.0f * (y * v.z - z * v.y), 2.0f * (z * v.x - x * v.z), 2.0f * (x * v.y - y * v.x) };
    return { v.x + w * t.x + (y * t.z - z * t.y), v.y + w * t.y + (z * t.x - x * t.z),
             v.z + w * t.z + (x * t.y - y * t.x) };
}

inline Vec3 InverseRotate(const float q[4], const Vec3& v) {
    const float inv[4] = { -q[0], -q[1], -q[2], q[3] };
    return Rotate(inv, v);
}

// The object keeps its place relative to the hand from the grab. GrabOffset: the actor position
// in the frame of the hand. HeldPos: the actor position for the hand pose.
inline Vec3 GrabOffset(const Vec3& handPos, const float handQuat[4], const Vec3& actorPos) {
    return InverseRotate(handQuat, { actorPos.x - handPos.x, actorPos.y - handPos.y, actorPos.z - handPos.z });
}

inline Vec3 HeldPos(const Vec3& handPos, const float handQuat[4], const Vec3& offset) {
    const Vec3 o = Rotate(handQuat, offset);
    return { handPos.x + o.x, handPos.y + o.y, handPos.z + o.z };
}

struct HandInput {
    bool gripHeld;
    bool blocked; // Another item uses this grip: the belt, the boomerang, or the bow string.
    Vec3 pos;     // Game units.
    int target;   // The candidate in reach, or -1.
};

struct Input {
    bool canGrab;  // Link can lift an object now.
    bool carrying; // Link lifts or carries the object of the last grab.
    HandInput hands[2];
    int firstHand; // When the two hands lift an object in the same tick, this hand gets it.
};

enum class Event { None, Grab, Throw, Drop };

struct Result {
    Event event = Event::None;
    int hand = -1;
    int target = -1;                    // Grab: the candidate.
    Vec3 pos = { 0.0f, 0.0f, 0.0f };    // Throw and Drop: the hand position.
    Vec3 velMps = { 0.0f, 0.0f, 0.0f }; // Throw: the hand velocity.
    bool inReach[2] = { false, false };    // The hand can lift an object.
    bool reachPulse[2] = { false, false }; // The hand came to an object in this tick.
};

class Hold {
  public:
    void AddSample(int hand, const Sample& s) {
        mHistory[hand].AddSample(s);
    }

    // Call when the hand is not tracked.
    void Clear(int hand) {
        mHistory[hand].Clear();
    }

    // The hand that holds the object, or -1.
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
            // The object is gone, or the game did not take the grab.
            mHand = -1;
        }
        bool reach[2] = { false, false };
        if (mHand >= 0) {
            const HandInput& h = in.hands[mHand];
            if (!h.gripHeld) {
                r = Release(h.pos);
            }
        } else if (in.canGrab && !in.carrying) {
            for (int i = 0; i < 2; i++) {
                const int hand = (i == 0) ? in.firstHand : (in.firstHand ^ 1);
                const HandInput& h = in.hands[hand];
                reach[hand] = !h.blocked && (h.target >= 0);
                r.reachPulse[hand] = reach[hand] && !mInReach[hand];
                if (reach[hand] && (mGripAge[hand] != kNotHeld) && (mGripAge[hand] <= kGripGraceTicks) &&
                    (r.event == Event::None)) {
                    r.event = Event::Grab;
                    r.hand = hand;
                    r.target = h.target;
                    mHand = hand;
                    // One push lifts one object.
                    mGripAge[hand] = kGripGraceTicks + 1;
                }
            }
        }
        for (int hand = 0; hand < 2; hand++) {
            mInReach[hand] = reach[hand];
            r.inReach[hand] = reach[hand];
        }
        return r;
    }

  private:
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

} // namespace VrCarry
