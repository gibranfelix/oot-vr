// Host unit tests for VrMeleeWeapon.h (pure rules, no game or OpenXR). The game build does not
// compile this file. Run it from the repository root:
//
//   g++ -std=c++17 -I port/soh/soh/Enhancements/vr-combat port/soh/tests/vr/VrMeleeWeaponTests.cpp -o /tmp/vr_melee_tests
//   /tmp/vr_melee_tests

#include "VrMeleeWeapon.h"

#include <cstdio>

using namespace VrMeleeWeapon;

static int sFailures = 0;

#define EXPECT(cond)                                                    \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::printf("%s:%d: FAILED: %s\n", __FILE__, __LINE__, #cond); \
            sFailures++;                                                \
        }                                                               \
    } while (0)

static const Speeds kDefaults = { 2.0f, 5.0f, 0.8f, 1.2f };

static void SwordsAndStickAreCovered() {
    EXPECT(Covered(kMaster));
    EXPECT(Covered(kKokiri));
    EXPECT(Covered(kBiggoron));
    EXPECT(Covered(kDekuStick));
}

static void HammerAndEmptyHandsAreNotCovered() {
    EXPECT(!Covered(kNone));
    EXPECT(!Covered(kHammer));
}

static void StickUsesVanillaStickDamage() {
    EXPECT(DmgFlags(kDekuStick, false, false) == 0x00000002);
    EXPECT(DmgFlags(kDekuStick, false, true) == 0x08000000);
}

static void SwordDamageIsUnchanged() {
    EXPECT(DmgFlags(kMaster, false, false) == 0x00000200);
    EXPECT(DmgFlags(kKokiri, false, true) == 0x02000000);
    EXPECT(DmgFlags(kBiggoron, false, false) == 0x00000400);
    EXPECT(DmgFlags(kBiggoron, true, false) == 0x00000100); // broken Giant's Knife
}

static void UnknownWeaponFallsBackToKokiri() {
    EXPECT(DmgFlags(kNone, false, false) == 0x00000100);
    EXPECT(DmgFlags(kHammer, false, false) == 0x00000100);
}

static void StickLengthFollowsBurnDown() {
    EXPECT(StickLengthModelUnits(1.0f) == 5000.0f);
    EXPECT(StickLengthModelUnits(0.5f) == 2500.0f);
    EXPECT(StickLengthModelUnits(0.0f) == kMinStickModelUnits);
}

static void SoftMovementDoesNotHit() {
    EXPECT(NextTier(kIdle, 1.0f, 1.0f, kDefaults) == kIdle);
    EXPECT(NextTier(kIdle, 3.0f, 3.0f, kDefaults) == kArmed);
}

static void FastSwingHits() {
    EXPECT(NextTier(kIdle, 6.0f, 2.0f, kDefaults) == kHot);
    EXPECT(NextTier(kArmed, 6.0f, 2.0f, kDefaults) == kHot);
}

static void WristFlickDoesNotHit() {
    EXPECT(NextTier(kIdle, 9.0f, 0.5f, kDefaults) == kArmed);
    EXPECT(NextTier(kArmed, 9.0f, 0.5f, kDefaults) == kArmed);
}

static void HotStaysHotUntilTheSwingDecays() {
    EXPECT(NextTier(kHot, 1.0f, 0.0f, kDefaults) == kHot);
    EXPECT(NextTier(kHot, 0.5f, 0.0f, kDefaults) == kIdle);
    EXPECT(NextTier(kArmed, 0.5f, 0.0f, kDefaults) == kIdle);
}

static void StickBreaksOnEveryVanillaStrike() {
    EXPECT(StickStrikeBreaks(true, false, false));
    EXPECT(StickStrikeBreaks(false, true, false));
    EXPECT(StickStrikeBreaks(false, false, true));
}

static void StickDoesNotBreakWithoutAStrike() {
    EXPECT(!StickStrikeBreaks(false, false, false));
}

int main() {
    SwordsAndStickAreCovered();
    HammerAndEmptyHandsAreNotCovered();
    StickUsesVanillaStickDamage();
    SwordDamageIsUnchanged();
    UnknownWeaponFallsBackToKokiri();
    StickLengthFollowsBurnDown();
    SoftMovementDoesNotHit();
    FastSwingHits();
    WristFlickDoesNotHit();
    HotStaysHotUntilTheSwingDecays();
    StickBreaksOnEveryVanillaStrike();
    StickDoesNotBreakWithoutAStrike();

    if (sFailures != 0) {
        std::printf("%d failure(s)\n", sFailures);
        return 1;
    }
    std::printf("All VrMeleeWeapon tests passed.\n");
    return 0;
}
