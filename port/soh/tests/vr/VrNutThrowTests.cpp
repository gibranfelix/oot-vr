// Host unit tests for VrNutThrow.h (pure math, no game or OpenXR). The game build does not
// compile this file. Run it from the repository root:
//
//   g++ -std=c++17 -I port/soh/soh/Enhancements/vr-combat port/soh/tests/vr/VrNutThrowTests.cpp -o /tmp/vr_nut_throw_tests
//   /tmp/vr_nut_throw_tests

#include "VrNutThrow.h"

#include <cmath>
#include <cstdio>

using namespace VrNutThrow;

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

constexpr int kRight = 1;
constexpr Vec3 kBelt = { 0.0f, 50.0f, 10.0f };
constexpr Vec3 kAway = { 0.0f, 70.0f, 30.0f };

static Input MakeInput(bool rightGrip, Vec3 rightPos, bool carrying) {
    Input in = {};
    in.beltReady = true;
    in.carrying = carrying;
    in.hands[0] = { false, kAway };
    in.hands[1] = { rightGrip, rightPos };
    in.beltPos = kBelt;
    in.grabRadius = 5.0f;
    in.firstHand = kRight;
    return in;
}

static void TestTheNutGoesBackWhenTheHandStopsCarrying() {
    Holder h;
    h.Update(MakeInput(false, kAway, false));
    h.Update(MakeInput(true, kBelt, false));
    EXPECT(h.Hand() == kRight);
    // VrNut.cpp stops carrying on the chord: the nut goes back to the belt, with no event.
    Input in = MakeInput(true, kAway, false);
    in.beltReady = false;
    const Result r = h.Update(in);
    EXPECT(r.event == Event::None);
    EXPECT(h.Hand() == -1);
}

static void TestThrowVelocityIsInGameUnitsForEachTick() {
    // 2 m/s at 20 ticks/s: 0.1 m in each tick, 4 units, times the gain.
    const Vec3 v = ThrowVelocity({ 0.0f, 2.0f, 0.0f }, 20.0f);
    EXPECT(Near(v.y, 4.0f * kThrowGain));
}

static void TestTheFastestThrowIsLimited() {
    const Vec3 v = ThrowVelocity({ 30.0f, 0.0f, 40.0f }, 20.0f);
    EXPECT(Near(VrHandThrow::Length(v), kMaxThrowSpeed));
    EXPECT(Near(v.x / v.z, 0.75f));
}

static void TestTheNutLandsBeforeTheFlightEnds() {
    // The highest throw goes up and comes back to the hand height, then falls to the floor.
    const float upAndDown = 2.0f * kMaxThrowSpeed / kNutGravity;
    const float fallFromHand = std::sqrt(2.0f * 2.0f * kThrowUnitsPerMeter / kNutGravity);
    EXPECT(upAndDown + fallFromHand < (float)kFlightTicks);
}

static void TestTheNutHasTheSameRealSizeForChildAndAdult() {
    EXPECT(Near(DrawScale(22.0f) * kNutModelDiameter / 22.0f, kNutDiameterM, 0.001f));
    EXPECT(Near(DrawScale(37.0f) * kNutModelDiameter / 37.0f, kNutDiameterM, 0.001f));
}

static void TestTheNutIsInFrontOfTheFist() {
    const float s = 40.0f;
    // -Z of the OpenXR grip pose points forward, along the controller.
    const float identity[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    Vec3 o = ForwardOffset(identity, s, kForwardOffsetM);
    EXPECT(Near(o.x, 0.0f) && Near(o.y, 0.0f) && Near(o.z, -kForwardOffsetM * s));
    // The hand points down (-90 degrees about +X).
    const float h = std::sqrt(0.5f);
    const float down[4] = { -h, 0.0f, 0.0f, h };
    o = ForwardOffset(down, s, kForwardOffsetM);
    EXPECT(Near(o.x, 0.0f) && Near(o.y, -kForwardOffsetM * s) && Near(o.z, 0.0f));
}

int main() {
    TestTheNutIsInFrontOfTheFist();
    TestTheNutGoesBackWhenTheHandStopsCarrying();
    TestThrowVelocityIsInGameUnitsForEachTick();
    TestTheFastestThrowIsLimited();
    TestTheNutLandsBeforeTheFlightEnds();
    TestTheNutHasTheSameRealSizeForChildAndAdult();
    if (sFailures == 0) {
        std::printf("VrNutThrow: all tests passed\n");
        return 0;
    }
    std::printf("VrNutThrow: %d failures\n", sFailures);
    return 1;
}
