// Host unit tests for VrSwordVoice.h. Run them from the repository root:
//
//   g++ -std=c++17 -I port/soh/soh/Enhancements/vr-combat port/soh/tests/vr/VrSwordVoiceTests.cpp -o /tmp/vr_sword_voice_tests
//   /tmp/vr_sword_voice_tests

#include "VrSwordVoice.h"

#include <cstdio>

using namespace VrSwordVoice;

static int sFailures = 0;

#define EXPECT(cond)                                                    \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::printf("%s:%d: FAILED: %s\n", __FILE__, __LINE__, #cond); \
            sFailures++;                                                \
        }                                                               \
    } while (0)

static void VrOffAlwaysShouts() {
    for (int mode = kNever; mode <= kAlways; mode++) {
        EXPECT(Plays(false, mode, false));
        EXPECT(Plays(false, mode, true));
    }
}

static void NeverIsSilent() {
    EXPECT(!Plays(true, kNever, false));
    EXPECT(!Plays(true, kNever, true));
}

static void SpinAndJumpShoutsOnlyOnStrongAttacks() {
    EXPECT(!Plays(true, kSpinAndJump, false));
    EXPECT(Plays(true, kSpinAndJump, true));
}

static void AlwaysShouts() {
    EXPECT(Plays(true, kAlways, false));
    EXPECT(Plays(true, kAlways, true));
}

static void DefaultIsTheOriginalGame() {
    EXPECT(kDefault == kAlways);
}

int main() {
    VrOffAlwaysShouts();
    NeverIsSilent();
    SpinAndJumpShoutsOnlyOnStrongAttacks();
    AlwaysShouts();
    DefaultIsTheOriginalGame();
    if (sFailures == 0) {
        std::printf("VrSwordVoice: all tests passed\n");
    }
    return sFailures == 0 ? 0 : 1;
}
