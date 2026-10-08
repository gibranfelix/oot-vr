#pragma once

// Pure rules behind physical melee (VrSwing.cpp): which weapons swing physically, their damage
// flags, the swing tier machine, and when the Deku Stick breaks. No game types, so the host tests
// in port/soh/tests/vr/VrMeleeWeaponTests.cpp can build it alone.

#include <cstdint>

namespace VrMeleeWeapon {

// Player_GetMeleeWeaponHeld values (z_player_lib.c).
constexpr int kNone = 0;
constexpr int kMaster = 1;
constexpr int kKokiri = 2;
constexpr int kBiggoron = 3; // also the Giant's Knife, broken or not
constexpr int kDekuStick = 4;
constexpr int kHammer = 5;

// Physical combat handles the swords and the Deku Stick. The hammer keeps vanilla button combat
// until its own issue.
inline bool Covered(int held) {
    return held == kMaster || held == kKokiri || held == kBiggoron || held == kDekuStick;
}

// [slash, jump-slash] dmgFlags, rows matching vanilla D_80854488 (z_player.c): Master, Kokiri
// (also the broken Giant's Knife), Biggoron, Deku Stick.
constexpr uint32_t kDmgFlags[4][2] = {
    { 0x00000200, 0x08000000 },
    { 0x00000100, 0x02000000 },
    { 0x00000400, 0x04000000 },
    { 0x00000002, 0x08000000 },
};

// Normal swings carry the slash flags; swings at or past the heavy speed carry the jump-slash
// flags (~2x in enemy damage tables).
inline uint32_t DmgFlags(int held, bool brokenKnife, bool heavy) {
    int row = brokenKnife ? 1 : held - 1;
    if (row < 0 || row > 3) {
        row = 1;
    }
    return kDmgFlags[row][heavy ? 1 : 0];
}

// Vanilla stick length in hand-model units: Player_PostLimbDrawGameplay sets the stick tip to
// unk_85C * 5000. unk_85C is 1 for a full stick and falls toward 0 while it burns down. The
// floor keeps the physical weapon a real segment on the last frame of a burn-out.
constexpr float kMinStickModelUnits = 100.0f;
inline float StickLengthModelUnits(float stickLength) {
    const float len = stickLength * 5000.0f;
    return (len > kMinStickModelUnits) ? len : kMinStickModelUnits;
}

// Swing tiers. IDLE below the arm speed; ARMED (windup — enemies see meleeWeaponState -1) above
// it; HOT (damage) at the hit speed, but only when the hand itself moves (a stationary-wrist
// flick spins the weapon fast but never hurts). Back to IDLE when the swing decays below the
// re-arm speed.
enum Tier { kIdle = 0, kArmed, kHot };

struct Speeds {
    float arm;
    float hit;
    float reArm;
    float minHand;
};

inline int NextTier(int tier, float tipSpeed, float handSpeed, const Speeds& s) {
    const bool handCommitted = handSpeed >= s.minHand;
    if (tier == kIdle) {
        if (tipSpeed >= s.hit && handCommitted) {
            return kHot;
        }
        return (tipSpeed >= s.arm) ? kArmed : kIdle;
    }
    if (tier == kArmed && tipSpeed >= s.hit && handCommitted) {
        tier = kHot;
    }
    if (tipSpeed < s.reArm) {
        tier = kIdle;
    }
    return tier;
}

// Vanilla breaks the Deku Stick when an attack hits something, bounces off something hard, or
// strikes a wall (func_80842DF4 -> func_80842AC4). Physical combat has the same three events: a
// damage quad hit, a damage quad bounce, and a damaging swing that touches the world.
inline bool StickStrikeBreaks(bool quadHit, bool quadBounced, bool hotWorldContact) {
    return quadHit || quadBounced || hotWorldContact;
}

} // namespace VrMeleeWeapon
