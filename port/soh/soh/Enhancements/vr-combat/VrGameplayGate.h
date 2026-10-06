#pragma once

// Pure rule behind VrCombat_InPlay (VrCombat.cpp): no game types, so the host tests in
// port/soh/tests/vr/VrGameplayGateTests.cpp can build it alone.

namespace VrGameplayGate {

constexpr int kDebugSaveFileNum = 0xFF;
// FileChoose_LoadGame starts Boss Rush with this fileNum and a temporary save, not a slot.
constexpr int kBossRushFileNum = 0xFE;

// The player plays the game: a save slot, the debug save, or Boss Rush. GameInteractor's
// IsSaveLoaded(true) is the same rule without Boss Rush. Boss Rush also needs its quest id,
// because Warping.cpp sets 0xFE for a moment outside Boss Rush.
inline bool InPlay(bool normalGameMode, int fileNum, bool bossRush) {
    if (!normalGameMode) {
        return false;
    }
    if ((fileNum >= 0 && fileNum <= 2) || fileNum == kDebugSaveFileNum) {
        return true;
    }
    return fileNum == kBossRushFileNum && bossRush;
}

} // namespace VrGameplayGate
