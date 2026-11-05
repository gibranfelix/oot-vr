// Host unit tests for VrCarry.h (pure math, no game or OpenXR). The game build does not compile
// this file. Run it from the repository root:
//
//   g++ -std=c++17 -I port/soh/soh/Enhancements/vr-combat port/soh/tests/vr/VrCarryTests.cpp -o /tmp/vr_carry_tests
//   /tmp/vr_carry_tests

#include "VrCarry.h"

#include <cmath>
#include <cstdio>

using namespace VrCarry;

static int sFailures = 0;

#define EXPECT(cond)                                                    \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::printf("%s:%d: FAILED: %s\n", __FILE__, __LINE__, #cond); \
            sFailures++;                                                \
        }                                                               \
    } while (0)

static bool Near(float a, float b, float eps = 0.01f) {
    return std::fabs(a - b) <= eps;
}

static bool NearV(const Vec3& a, const Vec3& b, float eps = 0.01f) {
    return Near(a.x, b.x, eps) && Near(a.y, b.y, eps) && Near(a.z, b.z, eps);
}

constexpr uint64_t kMs = 1000000ull;
constexpr int kRight = 1;
constexpr int kLeft = 0;
constexpr Vec3 kHand = { 0.0f, 30.0f, 20.0f };

// One tick of headset samples (8 ms apart) with the same velocity, at the hand position pos.
static void Feed(Hold& h, int hand, uint64_t* timeNs, Vec3 pos, Vec3 velMps, int count = 6) {
    for (int i = 0; i < count; i++) {
        *timeNs += 8 * kMs;
        h.AddSample(hand, { pos, velMps, *timeNs });
    }
}

static Input MakeInput(bool canGrab, bool carrying, bool leftGrip, int leftTarget, bool rightGrip,
                       int rightTarget) {
    Input in;
    in.canGrab = canGrab;
    in.carrying = carrying;
    in.hands[kLeft] = { leftGrip, false, kHand, leftTarget, false };
    in.hands[kRight] = { rightGrip, false, kHand, rightTarget, false };
    in.firstHand = kRight;
    return in;
}

static void TestDistanceToCylinder() {
    const Vec3 base = { 0.0f, 0.0f, 0.0f };
    // In the cylinder.
    EXPECT(Near(DistanceToCylinder({ 1.0f, 5.0f, 1.0f }, base, 10.0f, 20.0f), 0.0f));
    // To the side.
    EXPECT(Near(DistanceToCylinder({ 15.0f, 5.0f, 0.0f }, base, 10.0f, 20.0f), 5.0f));
    // Above the top.
    EXPECT(Near(DistanceToCylinder({ 0.0f, 23.0f, 0.0f }, base, 10.0f, 20.0f), 3.0f));
    // Below the bottom.
    EXPECT(Near(DistanceToCylinder({ 0.0f, -4.0f, 0.0f }, base, 10.0f, 20.0f), 4.0f));
    // Diagonal from the top edge: 3 out and 4 up.
    EXPECT(Near(DistanceToCylinder({ 13.0f, 24.0f, 0.0f }, base, 10.0f, 20.0f), 5.0f));
}

static void TestSnapToHand() {
    const Shape pot = ShapeOf(Kind::Pot); // Radius 9, height 26.
    const Vec3 actor = { 0.0f, 0.0f, 0.0f };
    // The hand is 5 units out of the side, at mid height: the pot moves 5 + depth to the hand.
    Vec3 p = SnapToHand({ 14.0f, 13.0f, 0.0f }, actor, pot, 2.0f);
    EXPECT(NearV(p, { 7.0f, 0.0f, 0.0f }));
    // The hand is 4 units above the top: the pot moves up 4 + depth.
    p = SnapToHand({ 0.0f, 30.0f, 0.0f }, actor, pot, 2.0f);
    EXPECT(NearV(p, { 0.0f, 6.0f, 0.0f }));
    // The hand is in the pot: no move.
    p = SnapToHand({ 1.0f, 10.0f, 1.0f }, actor, pot, 2.0f);
    EXPECT(NearV(p, actor));
}

static void TestPickTarget() {
    const Candidate c[3] = {
        { { 0.0f, 0.0f, 0.0f }, ShapeOf(Kind::Pot) },        // Radius 9, height 26.
        { { 30.0f, 0.0f, 0.0f }, ShapeOf(Kind::Crate) },     // Radius 12, height 27.
        { { 0.0f, 0.0f, 40.0f }, ShapeOf(Kind::SmallRock) }, // Radius 10, starts at y -2.
    };
    // No candidate in reach.
    EXPECT(PickTarget({ 0.0f, 100.0f, 0.0f }, c, 3, 4.0f) == -1);
    // The hand over the pot.
    EXPECT(PickTarget({ 0.0f, 28.0f, 0.0f }, c, 3, 4.0f) == 0);
    // Between the pot (edge at 9) and the crate (edge at 18): the crate is nearer.
    EXPECT(PickTarget({ 15.0f, 10.0f, 0.0f }, c, 3, 7.0f) == 1);
    // The rock collider starts 2 units below its position.
    EXPECT(PickTarget({ 0.0f, -1.5f, 40.0f }, c, 3, 0.0f) == 2);
    EXPECT(PickTarget({ 0.0f, -2.5f, 40.0f }, c, 3, 0.0f) == -1);
    // No candidates.
    EXPECT(PickTarget({ 0.0f, 0.0f, 0.0f }, c, 0, 4.0f) == -1);
}

static void TestWallInReach() {
    // The wall is the plane z = 50, its normal points to Link (-z).
    const float n[3] = { 0.0f, 0.0f, -1.0f };
    const float d = 50.0f;
    const Vec3 link = { 0.0f, 0.0f, 20.0f };
    EXPECT(WallInReach({ 5.0f, 40.0f, 48.0f }, link, n, d, 4.0f, 40.0f));
    // Too far from the wall.
    EXPECT(!WallInReach({ 5.0f, 40.0f, 40.0f }, link, n, d, 4.0f, 40.0f));
    // Too far from Link along the wall.
    EXPECT(!WallInReach({ 60.0f, 40.0f, 50.0f }, link, n, d, 4.0f, 40.0f));
}

static void TestPushAlong() {
    // The wall normal points to Link (-z). Push goes along +z.
    const float n[3] = { 0.0f, 0.0f, -1.0f };
    const Vec3 link = { 0.0f, 0.0f, 0.0f };
    EXPECT(Near(PushAlong({ 3.0f, 40.0f, 12.0f }, link, n), 12.0f));
    // The y part of the normal does not count.
    const float tilted[3] = { 0.0f, -0.6f, -0.8f };
    EXPECT(Near(PushAlong({ 0.0f, 40.0f, 10.0f }, link, tilted), 10.0f));
}

static void TestPushStick() {
    const float upm = 40.0f;
    const float push = kPushM * upm; // 3.2 units.
    PushStick p;
    p.Start(20.0f);
    EXPECT(p.Update(20.0f, upm) == 0);
    EXPECT(p.Update(20.0f + push * 0.5f, upm) == 0);
    // Hands forward: push.
    EXPECT(p.Update(20.0f + push, upm) == 1);
    // A small move back keeps the push (no flicker).
    EXPECT(p.Update(20.0f + push * 0.6f, upm) == 1);
    // Back near the grab point: stop.
    EXPECT(p.Update(20.0f + kStopM * upm * 0.5f, upm) == 0);
    // Hands back: pull.
    EXPECT(p.Update(20.0f - push, upm) == -1);
    // Start resets the direction.
    p.Start(0.0f);
    EXPECT(p.Update(0.0f, upm) == 0);
}

static void TestThrowVelocity() {
    const float tps = 20.0f;
    // The reference gravity gives the bomb throw: 4 m/s -> 4 * 40 / 20 * 1.8 = 14.4 units for each
    // tick.
    const Vec3 rock = ThrowVelocity({ 0.0f, 0.0f, 4.0f }, tps, ShapeOf(Kind::SmallRock).gravity);
    EXPECT(Near(rock.z, 14.4f));
    // The same hand speed throws all objects to the same distance: v * v / g is the same.
    const float g = ShapeOf(Kind::Bush).gravity;
    const Vec3 bush = ThrowVelocity({ 0.0f, 0.0f, 4.0f }, tps, g);
    EXPECT(Near(bush.z * bush.z / g, rock.z * rock.z / kRefGravity, 0.05f));
    // The fastest throw is limited.
    const Vec3 fast = ThrowVelocity({ 0.0f, 0.0f, 100.0f }, tps, kRefGravity);
    EXPECT(Near(VrHandThrow::Length(fast), kMaxThrowSpeed));
    // No gravity: the reference gravity.
    const Vec3 none = ThrowVelocity({ 0.0f, 0.0f, 4.0f }, tps, 0.0f);
    EXPECT(Near(none.z, 14.4f));
}

static void TestRotate() {
    // 90 degrees about +y: +x goes to -z.
    const float s = std::sqrt(0.5f);
    const float yaw90[4] = { 0.0f, s, 0.0f, s };
    EXPECT(NearV(Rotate(yaw90, { 1.0f, 0.0f, 0.0f }), { 0.0f, 0.0f, -1.0f }));
    // The inverse gives the vector back.
    const float q[4] = { 0.1825742f, 0.3651484f, 0.5477226f, 0.7302967f };
    const Vec3 v = { 3.0f, -2.0f, 5.0f };
    EXPECT(NearV(InverseRotate(q, Rotate(q, v)), v));
}

static void TestHeldPosKeepsGrabPlace() {
    const float s = std::sqrt(0.5f);
    const float q0[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    const Vec3 hand0 = { 10.0f, 30.0f, 0.0f };
    const Vec3 actor = { 10.0f, 18.0f, 4.0f };
    const Vec3 off = GrabOffset(hand0, q0, actor);
    // The same hand pose: the object does not move.
    EXPECT(NearV(HeldPos(hand0, q0, off), actor));
    // The hand moves: the object moves with it.
    EXPECT(NearV(HeldPos({ 15.0f, 40.0f, 0.0f }, q0, off), { 15.0f, 28.0f, 4.0f }));
    // The hand turns 90 degrees about +y: the offset (0, -12, 4) turns to (4, -12, 0).
    const float yaw90[4] = { 0.0f, s, 0.0f, s };
    EXPECT(NearV(HeldPos(hand0, yaw90, off), { 14.0f, 18.0f, 0.0f }));
}

static void TestDropSpeed() {
    const float upm = 40.0f;
    // Near the floor: the object is put down.
    EXPECT(Near(DropSpeed(2.0f, upm), 0.0f));
    EXPECT(Near(DropSpeed(kPutDownHeightM * upm, upm), 0.0f));
    // Higher: the object falls.
    EXPECT(Near(DropSpeed(30.0f, upm), kFallSpeed));
}

static void TestGrab() {
    Hold h;
    // The grip was held before the start: too old.
    Result r = h.Update(MakeInput(true, false, false, -1, true, 0));
    EXPECT(r.event == Event::None);
    // Release and push the grip again in reach.
    h.Update(MakeInput(true, false, false, -1, false, 0));
    r = h.Update(MakeInput(true, false, false, -1, true, 0));
    EXPECT(r.event == Event::Grab);
    EXPECT(r.hand == kRight);
    EXPECT(r.target == 0);
    EXPECT(h.Hand() == kRight);
}

static void TestGrabGrace() {
    Hold h;
    h.Update(MakeInput(true, false, false, -1, false, -1));
    // The grip is pushed, then the hand comes to the object 3 ticks later.
    h.Update(MakeInput(true, false, false, -1, true, -1));
    h.Update(MakeInput(true, false, false, -1, true, -1));
    h.Update(MakeInput(true, false, false, -1, true, -1));
    Result r = h.Update(MakeInput(true, false, false, -1, true, 2));
    EXPECT(r.event == Event::Grab);
    EXPECT(r.target == 2);

    // A grip that is held too long does not take the object.
    Hold late;
    late.Update(MakeInput(true, false, false, -1, false, -1));
    for (int i = 0; i < kGripGraceTicks + 2; i++) {
        late.Update(MakeInput(true, false, false, -1, true, -1));
    }
    r = late.Update(MakeInput(true, false, false, -1, true, 1));
    EXPECT(r.event == Event::None);
    EXPECT(r.inReach[kRight]);
}

static void TestNoGrabWhenLinkCannotLift() {
    Hold h;
    h.Update(MakeInput(false, false, false, -1, false, 0));
    Result r = h.Update(MakeInput(false, false, false, -1, true, 0));
    EXPECT(r.event == Event::None);
    EXPECT(!r.inReach[kRight]);
}

static void TestBlockedHandDoesNotGrab() {
    Hold h;
    Input in = MakeInput(true, false, false, -1, false, 0);
    in.hands[kRight].blocked = true;
    h.Update(in);
    in.hands[kRight].gripHeld = true;
    Result r = h.Update(in);
    EXPECT(r.event == Event::None);
    EXPECT(!r.inReach[kRight]);
}

static void TestFirstHandWins() {
    Hold h;
    h.Update(MakeInput(true, false, false, 0, false, 1));
    Result r = h.Update(MakeInput(true, false, true, 0, true, 1));
    EXPECT(r.event == Event::Grab);
    EXPECT(r.hand == kRight);
    EXPECT(r.target == 1);
}

static void TestReachPulse() {
    Hold h;
    Result r = h.Update(MakeInput(true, false, false, 0, false, -1));
    EXPECT(r.reachPulse[kLeft]);
    EXPECT(r.inReach[kLeft]);
    r = h.Update(MakeInput(true, false, false, 0, false, -1));
    EXPECT(!r.reachPulse[kLeft]);
    EXPECT(r.inReach[kLeft]);
}

static Hold Holding(uint64_t* timeNs) {
    Hold h;
    h.Update(MakeInput(true, false, false, -1, false, 0));
    h.Update(MakeInput(true, false, false, -1, true, 0));
    Feed(h, kRight, timeNs, kHand, { 0.0f, 0.0f, 0.0f });
    // The game lifts the object.
    h.Update(MakeInput(false, true, false, -1, true, -1));
    return h;
}

static void TestThrow() {
    uint64_t t = 0;
    Hold h = Holding(&t);
    Feed(h, kRight, &t, kHand, { 0.0f, 1.0f, 5.0f }, 10);
    Result r = h.Update(MakeInput(false, true, false, -1, false, -1));
    EXPECT(r.event == Event::Throw);
    EXPECT(r.hand == kRight);
    EXPECT(Near(r.velMps.z, 5.0f));
    EXPECT(h.Hand() == -1);
}

static void TestSlowReleaseDrops() {
    uint64_t t = 0;
    Hold h = Holding(&t);
    Feed(h, kRight, &t, kHand, { 0.0f, 0.0f, 0.5f });
    Result r = h.Update(MakeInput(false, true, false, -1, false, -1));
    EXPECT(r.event == Event::Drop);
    EXPECT(NearV(r.pos, kHand));
}

static void TestNoSamplesDrops() {
    Hold h;
    h.Update(MakeInput(true, false, false, -1, false, 0));
    h.Update(MakeInput(true, false, false, -1, true, 0));
    Result r = h.Update(MakeInput(false, true, false, -1, false, -1));
    EXPECT(r.event == Event::Drop);
}

static void TestObjectGoneResets() {
    uint64_t t = 0;
    Hold h = Holding(&t);
    // The bomb exploded in the hand.
    Result r = h.Update(MakeInput(false, false, false, -1, false, -1));
    EXPECT(r.event == Event::None);
    EXPECT(h.Hand() == -1);
}

static void TestReleaseWhileGameLifts() {
    // The game did not attach the object yet, but it lifts it: the release still counts.
    Hold h;
    h.Update(MakeInput(true, false, false, -1, false, 0));
    h.Update(MakeInput(true, false, false, -1, true, 0));
    Result r = h.Update(MakeInput(false, true, false, -1, false, -1));
    EXPECT(r.event == Event::Drop);
}

static void TestOtherHandNoSecondGrab() {
    uint64_t t = 0;
    Hold h = Holding(&t);
    // The left hand is in reach of an other object and pushes the grip: no grab.
    h.Update(MakeInput(false, true, false, 1, true, -1));
    Result r = h.Update(MakeInput(false, true, true, 1, true, -1));
    EXPECT(r.event == Event::None);
    EXPECT(h.Hand() == kRight);
}

static Input TwoHandInput(bool carrying, bool leftGrip, bool rightGrip, int leftTarget = 0, int rightTarget = 0) {
    Input in = MakeInput(!carrying, carrying, leftGrip, leftTarget, rightGrip, rightTarget);
    in.hands[kLeft].pos = { -10.0f, 30.0f, 20.0f };
    in.hands[kRight].pos = { 10.0f, 30.0f, 20.0f };
    in.hands[kLeft].twoHands = (leftTarget >= 0);
    in.hands[kRight].twoHands = (rightTarget >= 0);
    return in;
}

static void TestTwoHandGrab() {
    Hold h;
    h.Update(TwoHandInput(false, false, false));
    // The left grip first, the right grip 2 ticks later.
    Result r = h.Update(TwoHandInput(false, true, false));
    EXPECT(r.event == Event::None);
    EXPECT(r.inReach[kLeft]);
    h.Update(TwoHandInput(false, true, false));
    r = h.Update(TwoHandInput(false, true, true));
    EXPECT(r.event == Event::Grab);
    EXPECT(r.twoHands);
    EXPECT(r.target == 0);
    EXPECT(h.Hand() == kRight);
    EXPECT(h.TwoHands());
}

static void TestTwoHandNeedsBothHands() {
    Hold h;
    h.Update(TwoHandInput(false, false, false, -1, 0));
    Result r = h.Update(TwoHandInput(false, true, true, -1, 0));
    EXPECT(r.event == Event::None);
    EXPECT(r.inReach[kRight]);
    // Two different silver boulders.
    Hold other;
    other.Update(TwoHandInput(false, false, false, 0, 1));
    r = other.Update(TwoHandInput(false, true, true, 0, 1));
    EXPECT(r.event == Event::None);
}

static void TestTwoHandOldGrips() {
    Hold h;
    // The two grips were held before the hands got to the object.
    h.Update(TwoHandInput(false, false, false, -1, -1));
    for (int i = 0; i < kGripGraceTicks + 2; i++) {
        h.Update(TwoHandInput(false, true, true, -1, -1));
    }
    Result r = h.Update(TwoHandInput(false, true, true));
    EXPECT(r.event == Event::None);
}

static Hold HoldingTwoHands() {
    Hold h;
    h.Update(TwoHandInput(false, false, false));
    h.Update(TwoHandInput(false, true, true));
    h.Update(TwoHandInput(true, true, true, -1, -1));
    return h;
}

static void TestTwoHandThrowAverage() {
    uint64_t t = 0;
    Hold h = HoldingTwoHands();
    uint64_t tl = t;
    Feed(h, kLeft, &tl, { -10.0f, 30.0f, 20.0f }, { 0.0f, 0.0f, 4.0f }, 10);
    Feed(h, kRight, &t, { 10.0f, 30.0f, 20.0f }, { 0.0f, 0.0f, 6.0f }, 10);
    // One grip opens: the object leaves both hands.
    Result r = h.Update(TwoHandInput(true, false, true, -1, -1));
    EXPECT(r.event == Event::Throw);
    EXPECT(r.twoHands);
    EXPECT(Near(r.velMps.z, 5.0f));
    EXPECT(NearV(r.pos, { 0.0f, 30.0f, 20.0f }));
    EXPECT(h.Hand() == -1);
    EXPECT(!h.TwoHands());
}

static void TestTwoHandSlowReleaseDrops() {
    uint64_t t = 0;
    Hold h = HoldingTwoHands();
    uint64_t tl = t;
    Feed(h, kLeft, &tl, { -10.0f, 30.0f, 20.0f }, { 0.0f, 0.0f, 0.2f }, 10);
    Feed(h, kRight, &t, { 10.0f, 30.0f, 20.0f }, { 0.0f, 0.0f, 0.2f }, 10);
    Result r = h.Update(TwoHandInput(true, true, false, -1, -1));
    EXPECT(r.event == Event::Drop);
    EXPECT(NearV(r.pos, { 0.0f, 30.0f, 20.0f }));
}

static void TestForget() {
    Hold h;
    h.Update(TwoHandInput(false, false, false));
    h.Update(TwoHandInput(false, true, true));
    // The game does the lift and the throw (the golden gauntlet pillar).
    h.Forget();
    EXPECT(h.Hand() == -1);
    Result r = h.Update(TwoHandInput(true, false, false, -1, -1));
    EXPECT(r.event == Event::None);
}

int main() {
    TestDistanceToCylinder();
    TestSnapToHand();
    TestPickTarget();
    TestWallInReach();
    TestPushAlong();
    TestPushStick();
    TestThrowVelocity();
    TestRotate();
    TestHeldPosKeepsGrabPlace();
    TestDropSpeed();
    TestGrab();
    TestGrabGrace();
    TestNoGrabWhenLinkCannotLift();
    TestBlockedHandDoesNotGrab();
    TestFirstHandWins();
    TestReachPulse();
    TestThrow();
    TestSlowReleaseDrops();
    TestNoSamplesDrops();
    TestObjectGoneResets();
    TestReleaseWhileGameLifts();
    TestOtherHandNoSecondGrab();
    TestTwoHandGrab();
    TestTwoHandNeedsBothHands();
    TestTwoHandOldGrips();
    TestTwoHandThrowAverage();
    TestTwoHandSlowReleaseDrops();
    TestForget();
    if (sFailures == 0) {
        std::printf("All VrCarry tests passed.\n");
    }
    return sFailures == 0 ? 0 : 1;
}
