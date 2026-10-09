#pragma once

// The bomb on the belt (VrBomb.cpp). The belt and the grab are in VrBeltThrow.h. No game types.
// Tests: port/soh/tests/vr/VrBombThrowTests.cpp.

#include "VrBeltThrow.h"

namespace VrBombThrow {

using namespace VrBeltThrow;

// A medium throw (4 m/s) goes as far as the vanilla throw.
constexpr float kThrowGain = 1.8f;
// EnBom gravity, in units for each tick squared.
constexpr float kBombGravity = 1.2f;
// Units for each tick. The longest throw goes 2.5 times the vanilla distance (160 units).
constexpr float kMaxThrowSpeed = 21.9f;
// The bomb model is 14 units wide. In VR it is kBombDiameterM wide in the real world.
constexpr float kBombModelDiameter = 14.0f;
constexpr float kBombDiameterM = 0.15f;

// The draw scale of the bomb model. Never larger than the vanilla bomb.
inline float DrawScale(float unitsPerMeter) {
    const float k = kBombDiameterM * unitsPerMeter / kBombModelDiameter;
    return (k > 1.0f) ? 1.0f : k;
}

// The bomb center is this distance out of the palm, thus the closed hand does not go into it.
constexpr float kPalmOffsetM = 0.05f;

inline Vec3 PalmOffset(const float quat[4], bool leftHand, float unitsPerMeter) {
    return VrBeltThrow::PalmOffset(quat, leftHand, unitsPerMeter, kPalmOffsetM);
}

// The hand velocity in m/s to the bomb velocity in units for each tick.
inline Vec3 ThrowVelocity(const Vec3& velMps, float ticksPerSecond) {
    return VrBeltThrow::ThrowVelocity(velMps, ticksPerSecond, kThrowGain, kMaxThrowSpeed);
}

} // namespace VrBombThrow
