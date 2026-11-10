#pragma once

// When Link shouts during an attack in VR (gVrSwordVoice, issue #19). Host tests:
// port/soh/tests/vr/VrSwordVoiceTests.cpp.

namespace VrSwordVoice {

enum Mode { kNever = 0, kSpinAndJump = 1, kAlways = 2 };
constexpr int kDefault = kAlways;

// strong: the voice of a spin attack or a jump attack (NA_SE_VO_LI_SWORD_L).
inline bool Plays(bool vrOn, int mode, bool strong) {
    return !vrOn || mode == kAlways || (mode == kSpinAndJump && strong);
}

} // namespace VrSwordVoice
