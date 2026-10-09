#pragma once

// The Deku nut on the belt (VrNut.cpp). The belt and the grab are in VrBeltThrow.h. No game types.
// Tests: port/soh/tests/vr/VrNutThrowTests.cpp.

#include "VrBeltThrow.h"

namespace VrNutThrow {

using namespace VrBeltThrow;

// A medium throw (4 m/s) goes as far as the vanilla bomb throw.
constexpr float kThrowGain = 1.8f;
// Units for each tick squared.
constexpr float kNutGravity = 1.2f;
// Units for each tick. The nut is lighter than the bomb, thus the longest throw goes farther.
constexpr float kMaxThrowSpeed = 27.0f;
// After this number of ticks in the air, the nut disappears without a flash.
constexpr int kFlightTicks = 60;
// The nut model of the item drop (GID_NUTS) is approximately kNutModelDiameter units wide. In VR it
// is kNutDiameterM wide in the real world.
constexpr float kNutModelDiameter = 100.0f;
constexpr float kNutDiameterM = 0.06f;
// The nut center is this distance out of the palm, thus the closed hand does not go into it.
constexpr float kPalmOffsetM = 0.03f;

// The draw scale of the nut model.
inline float DrawScale(float unitsPerMeter) {
    return kNutDiameterM * unitsPerMeter / kNutModelDiameter;
}

// The hand velocity in m/s to the nut velocity in units for each tick.
inline Vec3 ThrowVelocity(const Vec3& velMps, float ticksPerSecond) {
    return VrBeltThrow::ThrowVelocity(velMps, ticksPerSecond, kThrowGain, kMaxThrowSpeed);
}

} // namespace VrNutThrow
