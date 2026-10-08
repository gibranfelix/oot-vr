// Host unit tests for VrAimMark.h (pure rule, no game or OpenXR). The game build does not
// compile this file. Run it from the repository root:
//
//   g++ -std=c++17 -I port/soh/soh/Enhancements/vr-combat port/soh/tests/vr/VrAimMarkTests.cpp -o /tmp/vr_aim_mark_tests
//   /tmp/vr_aim_mark_tests

#include "VrAimMark.h"

#include <cstdio>

using namespace VrAimMark;

static int sFailures = 0;

#define EXPECT(cond)                                                    \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::printf("%s:%d: FAILED: %s\n", __FILE__, __LINE__, #cond); \
            sFailures++;                                                \
        }                                                               \
    } while (0)

static Mark Hookshot(bool vrOn, bool holdColors, bool surfaceHolds) {
    return MarkFor(vrOn, holdColors, false, true, surfaceHolds);
}

static void VrOffWithDefaultsIsVanilla() {
    EXPECT(Hookshot(false, false, true) == Mark::Vanilla);
    EXPECT(Hookshot(false, false, false) == Mark::Vanilla);
}

static void VrOffKeepsTheSohOption() {
    EXPECT(Hookshot(false, true, true) == Mark::Holds);
    EXPECT(Hookshot(false, true, false) == Mark::CannotHold);
}

static void VrOffCustomColorIsCannotHold() {
    EXPECT(MarkFor(false, false, true, true, true) == Mark::CannotHold);
}

static void VrOnAlwaysShowsHoldColors() {
    EXPECT(Hookshot(true, false, true) == Mark::Holds);
    EXPECT(Hookshot(true, false, false) == Mark::CannotHold);
    EXPECT(Hookshot(true, true, true) == Mark::Holds);
}

// The SoH bow and boomerang reticles also use this reticle.
static void OtherWeaponsCannotHold() {
    EXPECT(MarkFor(true, false, false, false, true) == Mark::CannotHold);
    EXPECT(MarkFor(false, true, false, false, true) == Mark::CannotHold);
}

int main() {
    VrOffWithDefaultsIsVanilla();
    VrOffKeepsTheSohOption();
    VrOffCustomColorIsCannotHold();
    VrOnAlwaysShowsHoldColors();
    OtherWeaponsCannotHold();
    if (sFailures == 0) {
        std::printf("All VrAimMark tests passed.\n");
    }
    return sFailures == 0 ? 0 : 1;
}
