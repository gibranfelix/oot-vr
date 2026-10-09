#pragma once

// The game side of the belt (VrBomb.cpp, VrNut.cpp). The math is in VrBeltThrow.h.

#include "VrBeltThrow.h"

struct Player;

namespace VrBelt {

int SwordHand();
bool OffHandTriggerHeld();
float UnitsPerMeter();
// Link cannot take or hold a belt item: he talks, climbs, swims, carries an actor, or similar.
bool Blocked(Player* player);
// Adds the hand samples of this tick to the holder.
void Feed(VrBeltThrow::Holder& holder);
// The hands, the belt position, and the grab radius of this tick. raiseM: the item is this
// distance above the belt.
VrBeltThrow::Input MakeInput(Player* player, bool beltReady, bool carrying, float raiseM = 0.0f);
VrBeltThrow::Vec3 BeltPos(Player* player, float raiseM = 0.0f);
// The item center: palmOffsetM out of the palm, and forwardM in front of the fist. False when the
// hand is not tracked.
bool PosInHand(int hand, float palmOffsetM, float* outXyz, float forwardM = 0.0f);
// The haptics of the reach and the grab. Updates inReach.
void Haptics(const VrBeltThrow::Result& r, bool inReach[2]);

} // namespace VrBelt
