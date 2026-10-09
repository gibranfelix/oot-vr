// Host unit tests for VrBoomerangThrow.h (pure math, no game or OpenXR). The game build does not
// compile this file. Run it from the repository root:
//
//   g++ -std=c++17 -I port/soh/soh/Enhancements/vr-combat port/soh/tests/vr/VrBoomerangThrowTests.cpp -o /tmp/vr_boomerang_throw_tests
//   /tmp/vr_boomerang_throw_tests

#include "VrBoomerangThrow.h"

#include <cmath>
#include <cstdio>

using namespace VrBoomerangThrow;

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
constexpr Vec3 kForward = { 0.0f, 0.0f, 1.0f };

// One tick of headset samples (8 ms apart) with the same velocity.
static void Feed(Detector& d, uint64_t* timeNs, Vec3 velMps, int count = 6) {
    for (int i = 0; i < count; i++) {
        *timeNs += 8 * kMs;
        d.AddSample({ { 1.0f, 2.0f, 3.0f }, velMps, *timeNs });
    }
}

static Input Held(bool grip) {
    Input in;
    in.canThrow = true;
    in.gripHeld = grip;
    in.otherGripHeld = false;
    in.forward = kForward;
    return in;
}

static void TestFastReleaseThrows() {
    Detector d;
    uint64_t t = 0;
    Throw out;
    Feed(d, &t, { 0.0f, 0.0f, 1.0f });
    EXPECT(!d.Update(Held(true), &out));
    Feed(d, &t, { 0.0f, 0.0f, 5.0f });
    EXPECT(d.Update(Held(false), &out));
    EXPECT(Near(out.dir.z, 1.0f));
    EXPECT(Near(out.pos.x, 1.0f) && Near(out.pos.y, 2.0f) && Near(out.pos.z, 3.0f));
    EXPECT(out.range > kMinRange && out.range < kMaxRange);
}

static void TestSlowReleaseDoesNotThrow() {
    Detector d;
    uint64_t t = 0;
    Throw out;
    Feed(d, &t, { 0.0f, 0.0f, 0.5f });
    EXPECT(!d.Update(Held(true), &out));
    Feed(d, &t, { 0.0f, 0.0f, kMinThrowSpeedMps * 0.5f });
    EXPECT(!d.Update(Held(false), &out));
}

static void TestReleaseWithoutHoldDoesNotThrow() {
    Detector d;
    uint64_t t = 0;
    Throw out;
    Feed(d, &t, { 0.0f, 0.0f, 8.0f });
    EXPECT(!d.Update(Held(false), &out));
}

static void TestOneThrowForOneRelease() {
    Detector d;
    uint64_t t = 0;
    Throw out;
    EXPECT(!d.Update(Held(true), &out));
    Feed(d, &t, { 0.0f, 0.0f, 6.0f });
    EXPECT(d.Update(Held(false), &out));
    Feed(d, &t, { 0.0f, 0.0f, 6.0f });
    EXPECT(!d.Update(Held(false), &out));
}

static void TestBothGripsCancel() {
    Detector d;
    uint64_t t = 0;
    Throw out;
    EXPECT(!d.Update(Held(true), &out));
    Input both = Held(true);
    both.otherGripHeld = true;
    EXPECT(!d.Update(both, &out));
    Feed(d, &t, { 0.0f, 0.0f, 6.0f });
    EXPECT(!d.Update(Held(false), &out));
}

static void TestCannotThrowCancels() {
    Detector d;
    uint64_t t = 0;
    Throw out;
    EXPECT(!d.Update(Held(true), &out));
    Input busy = Held(true);
    busy.canThrow = false;
    EXPECT(!d.Update(busy, &out));
    Feed(d, &t, { 0.0f, 0.0f, 6.0f });
    EXPECT(!d.Update(Held(false), &out));
}

static void TestBackwardReleaseDoesNotThrow() {
    Detector d;
    uint64_t t = 0;
    Throw out;
    EXPECT(!d.Update(Held(true), &out));
    Feed(d, &t, { 0.0f, 0.0f, -6.0f });
    EXPECT(!d.Update(Held(false), &out));
}

static void TestDirectionIsThePeakNotTheFollowThrough() {
    Detector d;
    uint64_t t = 0;
    Throw out;
    EXPECT(!d.Update(Held(true), &out));
    // The throw goes forward and up, then the hand slows down and drops after the release.
    const float k = std::sqrt(0.5f);
    Feed(d, &t, { 0.0f, 5.0f * k, 5.0f * k }, 6);
    Feed(d, &t, { 0.0f, -1.0f, 0.5f }, 3);
    EXPECT(d.Update(Held(false), &out));
    EXPECT(Near(out.dir.y, k, 0.02f));
    EXPECT(Near(out.dir.z, k, 0.02f));
    const float len = std::sqrt(out.dir.x * out.dir.x + out.dir.y * out.dir.y + out.dir.z * out.dir.z);
    EXPECT(Near(len, 1.0f, 0.0001f));
}

static void TestOldSamplesAreIgnored() {
    Detector d;
    uint64_t t = 0;
    Throw out;
    EXPECT(!d.Update(Held(true), &out));
    // A fast move long before the release does not count.
    Feed(d, &t, { 0.0f, 0.0f, 8.0f }, 6);
    Feed(d, &t, { 0.0f, 0.0f, 0.2f }, 40);
    EXPECT(!d.Update(Held(false), &out));
}

static void TestRangeFollowsTheSpeed() {
    EXPECT(Near(RangeForSpeed(kMinThrowSpeedMps), kMinRange));
    EXPECT(Near(RangeForSpeed(kFullThrowSpeedMps), kMaxRange));
    EXPECT(Near(RangeForSpeed(kFullThrowSpeedMps * 3.0f), kMaxRange));
    const float mid = RangeForSpeed((kMinThrowSpeedMps + kFullThrowSpeedMps) * 0.5f);
    EXPECT(Near(mid, (kMinRange + kMaxRange) * 0.5f));
}

static void TestMovesKeepTheRange() {
    // Speed 24, R_UPDATE_RATE 3: 36 units in each move. The vanilla 360 units take 10 moves.
    EXPECT(MovesForRange(360.0f, 1.5f) == 10);
    EXPECT(MovesForRange(kMinRange, 1.5f) == 5);
    EXPECT(MovesForRange(kMaxRange, 1.5f) == 17);
    EXPECT(MovesForRange(1.0f, 1.5f) == 1);
}

static void TestSwapNeedsANewGripPush() {
    Detector d;
    uint64_t t = 0;
    Throw out;
    EXPECT(!d.Update(Held(true), &out));
    Input both = Held(true);
    both.otherGripHeld = true;
    EXPECT(!d.Update(both, &out));
    // The player releases the left grip first. The right grip does not arm again.
    EXPECT(!d.Update(Held(true), &out));
    Feed(d, &t, { 0.0f, 0.0f, 6.0f });
    EXPECT(!d.Update(Held(false), &out));
    // A new push of the grip arms again.
    EXPECT(!d.Update(Held(true), &out));
    Feed(d, &t, { 0.0f, 0.0f, 6.0f });
    EXPECT(d.Update(Held(false), &out));
}

static void TestClearForgetsOldSamples() {
    Detector d;
    uint64_t t = 0;
    Throw out;
    EXPECT(!d.Update(Held(true), &out));
    Feed(d, &t, { 0.0f, 0.0f, 6.0f });
    // The hand is not tracked: no samples in this tick.
    d.Clear();
    EXPECT(!d.Update(Held(false), &out));
}

static void TestTowardTargetInTheCone() {
    const Vec3 pos = { 0, 0, 0 };
    const Vec3 dir = { 0, 0, 1 };
    EXPECT(TowardTarget(pos, dir, { 0, 0, 500 }));
    // 20 degrees to the side, and 20 degrees up.
    EXPECT(TowardTarget(pos, dir, { 182, 0, 500 }));
    EXPECT(TowardTarget(pos, dir, { 0, 182, 500 }));
    // 45 degrees to the side, and behind.
    EXPECT(!TowardTarget(pos, dir, { 500, 0, 500 }));
    EXPECT(!TowardTarget(pos, dir, { 0, 0, -500 }));
    // The target is at the hand.
    EXPECT(TowardTarget(pos, dir, pos));
}

int main() {
    TestFastReleaseThrows();
    TestSlowReleaseDoesNotThrow();
    TestReleaseWithoutHoldDoesNotThrow();
    TestOneThrowForOneRelease();
    TestBothGripsCancel();
    TestCannotThrowCancels();
    TestBackwardReleaseDoesNotThrow();
    TestDirectionIsThePeakNotTheFollowThrough();
    TestOldSamplesAreIgnored();
    TestRangeFollowsTheSpeed();
    TestMovesKeepTheRange();
    TestSwapNeedsANewGripPush();
    TestClearForgetsOldSamples();
    TestTowardTargetInTheCone();

    if (sFailures == 0) {
        std::printf("VrBoomerangThrow: all tests passed\n");
        return 0;
    }
    std::printf("VrBoomerangThrow: %d failures\n", sFailures);
    return 1;
}
