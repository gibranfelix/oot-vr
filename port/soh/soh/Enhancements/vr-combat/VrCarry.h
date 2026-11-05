#pragma once

// The carry (VrCarry.cpp, #158). No game types.
// Tests: port/soh/tests/vr/VrCarryTests.cpp.

#include "VrBeltThrow.h"

#include <cmath>

namespace VrCarry {

using Vec3 = VrHandThrow::Vec3;
using Sample = VrHandThrow::Sample;

// The golden gauntlet pillar is not here: only the A button lifts it. Its lift is a cutscene, and
// the landing uses the animation.
enum class Kind { None, Pot, SmallRock, Bush, Crate, BombFlower, Bomb, Cucco, SilverBoulder, Block };

// #159, #160.
inline bool NeedsTwoHands(Kind kind) {
    return (kind == Kind::SilverBoulder) || (kind == Kind::Block);
}

// The collider cylinder and the gravity, in game units. The cylinder starts yShift above the actor.
struct Shape {
    float radius;
    float height;
    float yShift;
    float gravity; // Units for each tick squared, positive.
};

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
        case Kind::SilverBoulder:
            return { 55.0f, 70.0f, 0.0f, 2.5f }; // EnIshi
        case Kind::Block: // A wall, not a cylinder.
        case Kind::None:
            break;
    }
    return { 0.0f, 0.0f, 0.0f, 0.0f };
}

// The hand lifts an object at this distance from its collider.
constexpr float kGrabMarginM = 0.1f;
// A grip pushed up to 0.3 s before the hand gets to the object lifts it.
constexpr int kGripGraceTicks = VrBeltThrow::kGripGraceTicks;
// Slower than this, the release is a drop.
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

// The hand lifts the object before it touches it. The object moves to the hand: the hand goes
// depth into the collider. In the collider: no move. Returns the new actor position.
inline Vec3 SnapToHand(const Vec3& hand, const Vec3& actor, const Shape& shape, float depth) {
    const float baseY = actor.y + shape.yShift;
    float dx = hand.x - actor.x;
    float dz = hand.z - actor.z;
    const float xz = std::sqrt(dx * dx + dz * dz);
    Vec3 closest = hand;
    if (xz > shape.radius) {
        closest.x = actor.x + dx * shape.radius / xz;
        closest.z = actor.z + dz * shape.radius / xz;
    }
    closest.y = std::fmin(std::fmax(hand.y, baseY), baseY + shape.height);
    const Vec3 d = { hand.x - closest.x, hand.y - closest.y, hand.z - closest.z };
    const float dist = VrHandThrow::Length(d);
    if (dist <= 0.0f) {
        return actor;
    }
    const float k = (dist + depth) / dist;
    return { actor.x + d.x * k, actor.y + d.y * k, actor.z + d.z * k };
}

// The hand goes this far into the object at the lift.
constexpr float kSnapDepthM = 0.03f;

struct Candidate {
    Vec3 pos; // The actor position.
    Shape shape;
};

// The nearest candidate in reach, or -1. margin: game units.
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

// The hand is at the block wall. Plane: n . p + d = 0. reach: xz distance to Link.
inline bool WallInReach(const Vec3& hand, const Vec3& link, const float n[3], float d, float margin,
                        float reach) {
    const float dx = hand.x - link.x;
    const float dz = hand.z - link.z;
    const float plane = n[0] * hand.x + n[1] * hand.y + n[2] * hand.z + d;
    return (std::fabs(plane) <= margin) && (std::sqrt(dx * dx + dz * dz) <= reach);
}

constexpr float kWallReachM = 1.0f;

// The same hand speed throws all objects the same distance as a bomb.
constexpr float kRefGravity = 1.2f;
constexpr float kThrowGain = 1.8f;
constexpr float kMaxThrowSpeed = 21.9f;

// The hand velocity in m/s to the object velocity in units for each tick.
inline Vec3 ThrowVelocity(const Vec3& velMps, float ticksPerSecond, float gravity) {
    const float k = std::sqrt(((gravity > 0.0f) ? gravity : kRefGravity) / kRefGravity);
    return VrBeltThrow::ThrowVelocity(velMps, ticksPerSecond, kThrowGain * k, kMaxThrowSpeed * k);
}

// A drop nearer to the floor than this puts the object down. Higher, it falls.
constexpr float kPutDownHeightM = 0.15f;
// Units for each tick. At zero speed, ObjKibako puts itself down in the air.
constexpr float kFallSpeed = 0.2f;

inline float DropSpeed(float heightAboveFloor, float unitsPerMeter) {
    return (heightAboveFloor > kPutDownHeightM * unitsPerMeter) ? kFallSpeed : 0.0f;
}

// Block push (#160). Move the hands forward of the grab to push, back to pull. The game moves
// the block at its own speed.
constexpr float kPushM = 0.08f;
constexpr float kStopM = 0.04f;

// The hand distance from the eyes into the wall, on the floor plane. n: the wall normal, to Link.
inline float PushAlong(const Vec3& hand, const Vec3& eye, const float n[3]) {
    const float len = std::sqrt(n[0] * n[0] + n[2] * n[2]);
    if (len <= 0.0f) {
        return 0.0f;
    }
    return -((hand.x - eye.x) * n[0] + (hand.z - eye.z) * n[2]) / len;
}

// Works as the stick: 1 push, -1 pull, 0 stop.
class PushStick {
  public:
    void Start(float along) {
        mRef = along;
        mDir = 0;
    }

    int Update(float along, float unitsPerMeter) {
        const float d = along - mRef;
        if (d >= kPushM * unitsPerMeter) {
            mDir = 1;
        } else if (d <= -kPushM * unitsPerMeter) {
            mDir = -1;
        } else if (std::fabs(d) < kStopM * unitsPerMeter) {
            mDir = 0;
        }
        return mDir;
    }

  private:
    float mRef = 0.0f;
    int mDir = 0;
};

inline Vec3 Mid(const Vec3& a, const Vec3& b) {
    return { 0.5f * (a.x + b.x), 0.5f * (a.y + b.y), 0.5f * (a.z + b.z) };
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

// The object keeps its offset from the hand. GrabOffset is in the frame of the hand.
inline Vec3 GrabOffset(const Vec3& handPos, const float handQuat[4], const Vec3& actorPos) {
    return InverseRotate(handQuat, { actorPos.x - handPos.x, actorPos.y - handPos.y, actorPos.z - handPos.z });
}

inline Vec3 HeldPos(const Vec3& handPos, const float handQuat[4], const Vec3& offset) {
    const Vec3 o = Rotate(handQuat, offset);
    return { handPos.x + o.x, handPos.y + o.y, handPos.z + o.z };
}

struct HandInput {
    bool gripHeld;
    bool blocked; // The belt, the boomerang, or the bow string uses this grip.
    Vec3 pos;     // Game units.
    int target;   // The candidate in reach, or -1.
    bool twoHands; // The target needs two hands.
};

struct Input {
    bool canGrab;  // Link can lift an object now.
    bool carrying; // Link lifts or holds the object of the last grab.
    HandInput hands[2];
    int firstHand; // Wins when both hands grab in the same tick.
};

enum class Event { None, Grab, Throw, Drop };

struct Result {
    Event event = Event::None;
    int hand = -1;
    int target = -1;                    // Grab: the candidate.
    bool twoHands = false;
    Vec3 pos = { 0.0f, 0.0f, 0.0f };    // Throw and Drop: the hand position. Two hands: the midpoint.
    Vec3 velMps = { 0.0f, 0.0f, 0.0f }; // Throw: the hand velocity. Two hands: the mean.
    bool inReach[2] = { false, false };    // The hand can lift an object.
    bool reachPulse[2] = { false, false }; // The hand got to an object in this tick.
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

    // The hand that holds the object, or -1. With two hands: firstHand.
    int Hand() const {
        return mHand;
    }

    bool TwoHands() const {
        return mTwoHands;
    }

    // The game moves the object (block).
    void Forget() {
        mHand = -1;
        mTwoHands = false;
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
            Forget();
        }
        bool reach[2] = { false, false };
        if (mTwoHands) {
            // One open grip releases the object.
            if (!in.hands[0].gripHeld || !in.hands[1].gripHeld) {
                r = ReleaseTwoHands(in.hands[0].pos, in.hands[1].pos);
            }
        } else if (mHand >= 0) {
            const HandInput& h = in.hands[mHand];
            if (!h.gripHeld) {
                r = Release(h.pos);
            }
        } else if (in.canGrab && !in.carrying) {
            const HandInput& l = in.hands[0];
            const HandInput& rh = in.hands[1];
            if (!l.blocked && !rh.blocked && l.twoHands && (l.target >= 0) && (l.target == rh.target) &&
                (mGripAge[0] != kNotHeld) && (mGripAge[1] != kNotHeld) &&
                ((mGripAge[0] <= kGripGraceTicks) || (mGripAge[1] <= kGripGraceTicks))) {
                r.event = Event::Grab;
                r.hand = in.firstHand;
                r.target = l.target;
                r.twoHands = true;
                mHand = in.firstHand;
                mTwoHands = true;
                mGripAge[0] = mGripAge[1] = kGripGraceTicks + 1;
            }
            for (int i = 0; i < 2; i++) {
                const int hand = (i == 0) ? in.firstHand : (in.firstHand ^ 1);
                const HandInput& h = in.hands[hand];
                reach[hand] = !h.blocked && (h.target >= 0);
                r.reachPulse[hand] = reach[hand] && !mInReach[hand];
                if (reach[hand] && !h.twoHands && (mGripAge[hand] != kNotHeld) &&
                    (mGripAge[hand] <= kGripGraceTicks) && (r.event == Event::None)) {
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
    Result ReleaseTwoHands(const Vec3& left, const Vec3& right) {
        Result r;
        r.hand = mHand;
        r.twoHands = true;
        r.pos = Mid(left, right);
        Vec3 v[2];
        Vec3 pos[2];
        if (mHistory[0].Release(&v[0], &pos[0]) && mHistory[1].Release(&v[1], &pos[1])) {
            r.velMps = Mid(v[0], v[1]);
        }
        r.event = (VrHandThrow::Length(r.velMps) < kMinThrowSpeedMps) ? Event::Drop : Event::Throw;
        if (r.event == Event::Drop) {
            r.velMps = { 0.0f, 0.0f, 0.0f };
        }
        Forget();
        return r;
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
    bool mTwoHands = false;
    // Ticks since the push of the grip. A grip that is held at the start is too old.
    static constexpr int kNotHeld = -1;
    int mGripAge[2] = { kGripGraceTicks + 1, kGripGraceTicks + 1 };
    bool mInReach[2] = { false, false };
};

} // namespace VrCarry
