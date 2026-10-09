// Host unit tests for VrBombThrow.h (pure math, no game or OpenXR). The game build does not
// compile this file. Run it from the repository root:
//
//   g++ -std=c++17 -I port/soh/soh/Enhancements/vr-combat port/soh/tests/vr/VrBombThrowTests.cpp -o /tmp/vr_bomb_throw_tests
//   /tmp/vr_bomb_throw_tests

#include "VrBombThrow.h"

#include <cmath>
#include <cstdio>

using namespace VrBombThrow;

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

constexpr uint64_t kMs = 1000000ull;
constexpr int kRight = 1;
constexpr int kLeft = 0;
constexpr Vec3 kBelt = { 0.0f, 50.0f, 10.0f };
constexpr Vec3 kNearBelt = { 1.0f, 51.0f, 11.0f };
constexpr Vec3 kAway = { 0.0f, 70.0f, 30.0f };
constexpr float kRadius = 5.0f;

// One tick of headset samples (8 ms apart) with the same velocity, at the hand position pos.
static void Feed(Holder& h, int hand, uint64_t* timeNs, Vec3 pos, Vec3 velMps, int count = 6) {
    for (int i = 0; i < count; i++) {
        *timeNs += 8 * kMs;
        h.AddSample(hand, { pos, velMps, *timeNs });
    }
}

// The bomb is on the belt. Both hands are away, and no grip is held.
static Input Idle() {
    Input in;
    in.beltReady = true;
    in.carrying = false;
    in.swapChord = false;
    in.hands[kLeft] = { false, kAway };
    in.hands[kRight] = { false, kAway };
    in.beltPos = kBelt;
    in.grabRadius = kRadius;
    in.firstHand = kRight;
    return in;
}

static Input WithHand(Input in, int hand, Vec3 pos, bool grip) {
    in.hands[hand] = { grip, pos };
    return in;
}

// After a grab, the game puts the bomb in Link's hands.
static Input Carrying(Input in) {
    in.beltReady = false;
    in.carrying = true;
    return in;
}

// Grab with the right hand: the first update releases the grip, the second pushes it at the belt.
static void Grab(Holder& h, int hand = kRight) {
    EXPECT(h.Update(WithHand(Idle(), hand, kNearBelt, false)).event == Event::None);
    const Result r = h.Update(WithHand(Idle(), hand, kNearBelt, true));
    EXPECT(r.event == Event::Grab);
    EXPECT(r.hand == hand);
    EXPECT(h.Hand() == hand);
}

static void TestGripAtTheBeltTakesTheBomb() {
    Holder h;
    EXPECT(h.Hand() < 0);
    Grab(h);
    // The game spawns the bomb. The grip stays held: the bomb stays in the hand.
    const Result r = h.Update(Carrying(WithHand(Idle(), kRight, kAway, true)));
    EXPECT(r.event == Event::None);
    EXPECT(h.Hand() == kRight);
}

static void TestEitherHandTakesTheBomb() {
    Holder h;
    Grab(h, kLeft);
}

static void TestTheFirstHandWinsATie() {
    Holder h;
    Input in = WithHand(WithHand(Idle(), kLeft, kNearBelt, false), kRight, kNearBelt, false);
    h.Update(in);
    in = WithHand(WithHand(Idle(), kLeft, kNearBelt, true), kRight, kNearBelt, true);
    in.firstHand = kLeft;
    const Result r = h.Update(in);
    EXPECT(r.event == Event::Grab);
    EXPECT(r.hand == kLeft);
}

static void TestGripAwayFromTheBeltDoesNotTakeTheBomb() {
    Holder h;
    EXPECT(h.Update(WithHand(Idle(), kRight, kAway, true)).event == Event::None);
    // The grip is still held when the hand comes to the belt: no grab.
    EXPECT(h.Update(WithHand(Idle(), kRight, kNearBelt, true)).event == Event::None);
    EXPECT(h.Hand() < 0);
    // A new push of the grip at the belt takes the bomb.
    Grab(h);
}

static void TestGripHeldAtTheStartDoesNotTakeTheBomb() {
    Holder h;
    EXPECT(h.Update(WithHand(Idle(), kRight, kNearBelt, true)).event == Event::None);
}

static void TestNoGrabWhenTheBeltIsNotReady() {
    Holder h;
    h.Update(WithHand(Idle(), kRight, kNearBelt, false));
    Input in = WithHand(Idle(), kRight, kNearBelt, true);
    in.beltReady = false;
    EXPECT(h.Update(in).event == Event::None);
    EXPECT(h.Hand() < 0);
}

static void TestTheHandFeelsTheBelt() {
    Holder h;
    Result r = h.Update(Idle());
    EXPECT(!r.reachPulse[kLeft] && !r.reachPulse[kRight]);
    r = h.Update(WithHand(Idle(), kRight, kNearBelt, false));
    EXPECT(r.reachPulse[kRight] && !r.reachPulse[kLeft]);
    // One pulse when the hand comes in, not one in each tick.
    r = h.Update(WithHand(Idle(), kRight, kNearBelt, false));
    EXPECT(!r.reachPulse[kRight]);
    h.Update(Idle());
    r = h.Update(WithHand(Idle(), kRight, kNearBelt, false));
    EXPECT(r.reachPulse[kRight]);
    // No pulse when the belt is empty.
    h.Update(Idle());
    Input in = WithHand(Idle(), kRight, kNearBelt, false);
    in.beltReady = false;
    EXPECT(!h.Update(in).reachPulse[kRight]);
}

static void TestFastReleaseThrows() {
    Holder h;
    uint64_t t = 0;
    Grab(h);
    const Vec3 hand = { 2.0f, 60.0f, 20.0f };
    Feed(h, kRight, &t, hand, { 0.0f, 2.0f, 5.0f });
    const Result r = h.Update(Carrying(WithHand(Idle(), kRight, hand, false)));
    EXPECT(r.event == Event::Throw);
    EXPECT(r.hand == kRight);
    EXPECT(Near(r.velMps.y, 2.0f) && Near(r.velMps.z, 5.0f));
    EXPECT(Near(r.pos.x, hand.x) && Near(r.pos.y, hand.y) && Near(r.pos.z, hand.z));
    EXPECT(h.Hand() < 0);
}

static void TestBackwardThrowGoes() {
    Holder h;
    uint64_t t = 0;
    Grab(h);
    Feed(h, kRight, &t, kAway, { 0.0f, 0.0f, -5.0f });
    const Result r = h.Update(Carrying(WithHand(Idle(), kRight, kAway, false)));
    EXPECT(r.event == Event::Throw);
    EXPECT(Near(r.velMps.z, -5.0f));
}

static void TestSlowReleaseDropsTheBomb() {
    Holder h;
    uint64_t t = 0;
    Grab(h);
    Feed(h, kRight, &t, kAway, { 0.0f, 0.0f, kMinThrowSpeedMps * 0.5f });
    const Vec3 hand = { 3.0f, 61.0f, 31.0f };
    const Result r = h.Update(Carrying(WithHand(Idle(), kRight, hand, false)));
    EXPECT(r.event == Event::Drop);
    EXPECT(Near(r.pos.x, hand.x) && Near(r.pos.y, hand.y) && Near(r.pos.z, hand.z));
    EXPECT(Near(r.velMps.x, 0.0f) && Near(r.velMps.y, 0.0f) && Near(r.velMps.z, 0.0f));
}

static void TestReleaseWithoutSamplesDropsTheBomb() {
    Holder h;
    Grab(h);
    h.Clear(kRight);
    const Result r = h.Update(Carrying(WithHand(Idle(), kRight, kAway, false)));
    EXPECT(r.event == Event::Drop);
    EXPECT(Near(r.pos.y, kAway.y) && Near(r.pos.z, kAway.z));
}

static void TestOnlyTheHoldingHandReleases() {
    Holder h;
    uint64_t t = 0;
    Grab(h);
    Feed(h, kLeft, &t, kAway, { 0.0f, 0.0f, 6.0f });
    // The other grip goes up and down: nothing happens.
    Input in = Carrying(WithHand(Idle(), kRight, kAway, true));
    EXPECT(h.Update(WithHand(in, kLeft, kAway, true)).event == Event::None);
    EXPECT(h.Update(WithHand(in, kLeft, kAway, false)).event == Event::None);
    EXPECT(h.Hand() == kRight);
}

static void TestSwapChordDropsTheBomb() {
    Holder h;
    uint64_t t = 0;
    Grab(h);
    Feed(h, kRight, &t, kAway, { 0.0f, 0.0f, 6.0f });
    Input in = Carrying(WithHand(Idle(), kRight, kAway, true));
    in.swapChord = true;
    const Result r = h.Update(in);
    EXPECT(r.event == Event::Drop);
    EXPECT(h.Hand() < 0);
    // The release of the grip after the drop does not throw.
    in.swapChord = false;
    in.carrying = false;
    EXPECT(h.Update(WithHand(in, kRight, kAway, false)).event == Event::None);
}

static void TestBombThatIsGoneFreesTheHand() {
    Holder h;
    uint64_t t = 0;
    Grab(h);
    // The bomb exploded in the hand. The grip is still held.
    EXPECT(h.Update(WithHand(Idle(), kRight, kAway, true)).event == Event::None);
    EXPECT(h.Hand() < 0);
    Feed(h, kRight, &t, kAway, { 0.0f, 0.0f, 6.0f });
    EXPECT(h.Update(WithHand(Idle(), kRight, kAway, false)).event == Event::None);
}

static void TestOneReleaseForOneGrab() {
    Holder h;
    uint64_t t = 0;
    Grab(h);
    Feed(h, kRight, &t, kAway, { 0.0f, 0.0f, 6.0f });
    EXPECT(h.Update(Carrying(WithHand(Idle(), kRight, kAway, false))).event == Event::Throw);
    // The game has not taken the throw yet: Link still carries the bomb.
    Feed(h, kRight, &t, kAway, { 0.0f, 0.0f, 6.0f });
    EXPECT(h.Update(Carrying(WithHand(Idle(), kRight, kAway, false))).event == Event::None);
}

static void TestBeltIsBelowTheEyesInFrontOfLink() {
    const float s = 40.0f;
    const Vec3 eye = { 5.0f, 100.0f, -5.0f };
    // Link faces +z (yaw 0).
    Vec3 a = BeltAnchor(eye, 0.0f, s);
    EXPECT(Near(a.x, eye.x));
    EXPECT(Near(a.y, eye.y - kBeltBelowEyesM * s));
    EXPECT(Near(a.z, eye.z + kBeltForwardM * s));
    // Link faces +x.
    a = BeltAnchor(eye, 0.5f * (float)M_PI, s);
    EXPECT(Near(a.x, eye.x + kBeltForwardM * s));
    EXPECT(Near(a.z, eye.z));
}

static void TestThrowVelocityIsInGameUnitsForEachTick() {
    // 2 m/s at 35 units for each meter and 20 ticks for each second: 3.5 units for each tick.
    Vec3 v = ThrowVelocity({ 0.0f, 2.0f, 0.0f }, 35.0f, 20.0f);
    EXPECT(Near(v.y, 3.5f * kThrowGain));
    EXPECT(Near(v.x, 0.0f) && Near(v.z, 0.0f));
    // A very fast throw is clamped. The direction does not change.
    v = ThrowVelocity({ 30.0f, 0.0f, 40.0f }, 35.0f, 20.0f);
    EXPECT(Near(std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z), kMaxThrowSpeed));
    EXPECT(Near(v.x / v.z, 0.75f));
}

static void TestMaxThrowGoesTwoTimesTheVanillaDistance() {
    // Vanilla: 8 forward and 12 up, gravity 1.2: 20 ticks in the air, 160 units.
    const float vanilla = 8.0f * (2.0f * 12.0f / kBombGravity);
    // The best angle (45 degrees) at the max speed, from the same height.
    const float best = kMaxThrowSpeed * kMaxThrowSpeed / kBombGravity;
    EXPECT(best >= 1.9f * vanilla && best <= 2.1f * vanilla);
}

int main() {
    TestGripAtTheBeltTakesTheBomb();
    TestEitherHandTakesTheBomb();
    TestTheFirstHandWinsATie();
    TestGripAwayFromTheBeltDoesNotTakeTheBomb();
    TestGripHeldAtTheStartDoesNotTakeTheBomb();
    TestNoGrabWhenTheBeltIsNotReady();
    TestTheHandFeelsTheBelt();
    TestFastReleaseThrows();
    TestBackwardThrowGoes();
    TestSlowReleaseDropsTheBomb();
    TestReleaseWithoutSamplesDropsTheBomb();
    TestOnlyTheHoldingHandReleases();
    TestSwapChordDropsTheBomb();
    TestBombThatIsGoneFreesTheHand();
    TestOneReleaseForOneGrab();
    TestBeltIsBelowTheEyesInFrontOfLink();
    TestThrowVelocityIsInGameUnitsForEachTick();
    TestMaxThrowGoesTwoTimesTheVanillaDistance();

    if (sFailures == 0) {
        std::printf("VrBombThrow: all tests passed\n");
        return 0;
    }
    std::printf("VrBombThrow: %d failures\n", sFailures);
    return 1;
}
