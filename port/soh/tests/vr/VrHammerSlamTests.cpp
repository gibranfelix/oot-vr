// Host unit tests for VrHammerSlam.h (pure rule, no game or OpenXR). The game build does not
// compile this file. Run it from the repository root:
//
//   g++ -std=c++17 -I port/soh/soh/Enhancements/vr-combat port/soh/tests/vr/VrHammerSlamTests.cpp -o /tmp/vr_hammer_tests
//   /tmp/vr_hammer_tests

#include "VrHammerSlam.h"

#include <cstdio>

using namespace VrHammerSlam;

static int sFailures = 0;

#define EXPECT(cond)                                                    \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::printf("%s:%d: FAILED: %s\n", __FILE__, __LINE__, #cond); \
            sFailures++;                                                \
        }                                                               \
    } while (0)

static const Params kParams = {};

// The head of the hammer touches level floor at the height of Link's feet.
static Sample OnFloor(float downSpeedMps) {
    Sample s;
    s.headDownSpeedMps = downSpeedMps;
    s.headOnSurface = true;
    s.surfaceNormalY = 1.0f;
    s.surfaceYRelFeet = 0.0f;
    return s;
}

static Sample InAir(float downSpeedMps) {
    Sample s = OnFloor(downSpeedMps);
    s.headOnSurface = false;
    return s;
}

static void FastHitOnFloorSlams() {
    Detector d;
    EXPECT(d.Update(kParams, OnFloor(6.0f)));
}

static void SoftTouchDoesNotSlam() {
    Detector d;
    EXPECT(!d.Update(kParams, OnFloor(1.0f)));
}

static void FastMotionInAirDoesNotSlam() {
    Detector d;
    EXPECT(!d.Update(kParams, InAir(8.0f)));
}

static void UpwardMotionOnFloorDoesNotSlam() {
    // A fast lift from the floor reads as negative down speed.
    Detector d;
    EXPECT(!d.Update(kParams, OnFloor(-6.0f)));
}

static void WallHitDoesNotSlam() {
    Detector d;
    Sample s = OnFloor(6.0f);
    s.surfaceNormalY = 0.1f;
    EXPECT(!d.Update(kParams, s));
}

static void FloorFarFromFeetDoesNotSlam() {
    // The original game starts the shock wave only when the floor is within 40 units of Link's
    // feet: no shock wave on a ledge below Link or on a table at chest height.
    Detector d;
    Sample below = OnFloor(6.0f);
    below.surfaceYRelFeet = -45.0f;
    EXPECT(!d.Update(kParams, below));
    Sample above = OnFloor(6.0f);
    above.surfaceYRelFeet = 45.0f;
    EXPECT(!d.Update(kParams, above));
    Sample near = OnFloor(6.0f);
    near.surfaceYRelFeet = -30.0f;
    EXPECT(d.Update(kParams, near));
}

static void OneSlamUntilTheHeadLeavesTheSurface() {
    Detector d;
    EXPECT(d.Update(kParams, OnFloor(6.0f)));
    // The head stays on the floor, and the tracking still reports speed: no second slam.
    EXPECT(!d.Update(kParams, OnFloor(6.0f)));
    EXPECT(!d.Update(kParams, OnFloor(0.0f)));
    // Lift, then hit again.
    EXPECT(!d.Update(kParams, InAir(0.5f)));
    EXPECT(d.Update(kParams, OnFloor(6.0f)));
}

static void SoftTouchThenFastHitWithoutLiftSlams() {
    // A head that rests on the floor is not latched: only a slam latches.
    Detector d;
    EXPECT(!d.Update(kParams, OnFloor(0.2f)));
    EXPECT(d.Update(kParams, OnFloor(6.0f)));
}

static void ResetClearsTheLatch() {
    Detector d;
    EXPECT(d.Update(kParams, OnFloor(6.0f)));
    d.Reset();
    EXPECT(d.Update(kParams, OnFloor(6.0f)));
}

static void ThresholdComesFromParams() {
    Params p;
    p.minDownSpeedMps = 10.0f;
    Detector d;
    EXPECT(!d.Update(p, OnFloor(6.0f)));
    EXPECT(d.Update(p, OnFloor(10.0f)));
}

int main() {
    FastHitOnFloorSlams();
    SoftTouchDoesNotSlam();
    FastMotionInAirDoesNotSlam();
    UpwardMotionOnFloorDoesNotSlam();
    WallHitDoesNotSlam();
    FloorFarFromFeetDoesNotSlam();
    OneSlamUntilTheHeadLeavesTheSurface();
    SoftTouchThenFastHitWithoutLiftSlams();
    ResetClearsTheLatch();
    ThresholdComesFromParams();

    if (sFailures != 0) {
        std::printf("%d check(s) failed\n", sFailures);
        return 1;
    }
    std::printf("All VrHammerSlam tests passed.\n");
    return 0;
}
