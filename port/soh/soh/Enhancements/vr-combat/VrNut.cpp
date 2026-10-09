extern "C" {
#include "z64.h"
#include "macros.h"
#include "functions.h"
#include "variables.h"
#include "overlays/actors/ovl_En_Arrow/z_en_arrow.h"
extern PlayState* gPlayState;
}

#include "VrCombat.h"
#include "VrBelt.h"
#include "VrNutThrow.h"

#include <vr_interface.h>

#include <cmath>

// The Deku nut belt (#99). Read "Deku nut belt" in docs/architecture.md.

namespace {

VrNutThrow::Holder sHolder;
bool sInReach[2] = { false, false };
// A new nut comes to the belt this number of ticks after the last nut left the hand.
constexpr int kRefillTicks = 10;
int sRefillTicks = kRefillTicks;

// The vanilla game does not let Link use a nut in a room of type ROOM_BEHAVIOR_TYPE1_2.
bool LinkCanHoldNut(Player* player) {
    return !VrBelt::Blocked(player) && (gPlayState->roomCtx.curRoom.behaviorType1 != ROOM_BEHAVIOR_TYPE1_2) &&
           (AMMO(ITEM_NUT) > 0);
}

bool BeltReady(Player* player) {
    return (gPlayState != NULL) && VrCombat_NutUsesBelt(player) && (sHolder.Hand() < 0) && LinkCanHoldNut(player) &&
           (sRefillTicks == 0);
}

// The nut flies as an EnArrow nut. It flashes when it touches a wall or the floor.
void SpawnNut(Player* player, const VrNutThrow::Result& r) {
    Vec3f pos;
    if (!VrBelt::PosInHand(r.hand, VrNutThrow::kPalmOffsetM, &pos.x)) {
        pos = { r.pos.x, r.pos.y, r.pos.z };
    }
    VrNutThrow::Vec3 v = { 0.0f, 0.0f, 0.0f };
    const bool thrown = (r.event == VrNutThrow::Event::Throw);
    if (thrown) {
        v = VrNutThrow::ThrowVelocity(r.velMps, 60.0f / R_UPDATE_RATE);
    }
    // The hand velocity does not contain the walk of Link.
    Vec3f velocity = { v.x + player->actor.velocity.x, v.y, v.z + player->actor.velocity.z };
    Vec3f zero = { 0.0f, 0.0f, 0.0f };
    Actor* nut = Actor_Spawn(&gPlayState->actorCtx, gPlayState, ACTOR_EN_ARROW, pos.x, pos.y, pos.z, 0,
                             Math_Vec3f_Yaw(&zero, &velocity), 0, ARROW_NUT);
    if (nut == NULL) {
        return;
    }
    ((EnArrow*)nut)->vrFromHand = true;
    nut->speedXZ = sqrtf(SQ(velocity.x) + SQ(velocity.z));
    nut->velocity.y = velocity.y;
    nut->gravity = -VrNutThrow::kNutGravity;
    Inventory_ChangeAmmo(ITEM_NUT, -1);
    if (thrown) {
        Player_PlaySfx(&player->actor, NA_SE_PL_THROW);
        VR_TriggerHaptic(r.hand, 0.35f, 0.0f, 25.0f);
    }
}

} // namespace

void VrCombat::Nut_OnPlayerUpdate(Player* player) {
    if (!VR_IsInitialized() || (player == NULL) || (gPlayState == NULL)) {
        return;
    }
    VrBelt::Feed(sHolder);

    // The nut in the hand has no actor. When Link cannot hold it, or on the two grips that draw the
    // sword, the nut goes back to the belt. The player does not spend it.
    const bool carrying = (sHolder.Hand() >= 0) && VrCombat_NutUsesBelt(player) && LinkCanHoldNut(player) &&
                          !VrItemSelect_SwapChordHeld();
    if (carrying || !VrCombat_NutUsesBelt(player)) {
        sRefillTicks = kRefillTicks;
    } else if (sRefillTicks > 0) {
        sRefillTicks--;
    }

    const VrNutThrow::Result r = sHolder.Update(VrBelt::MakeInput(player, BeltReady(player), carrying, VrNutThrow::kBeltRaiseM));
    VrBelt::Haptics(r, sInReach);
    if ((r.event == VrNutThrow::Event::Throw) || (r.event == VrNutThrow::Event::Drop)) {
        SpawnNut(player, r);
    }
}

extern "C" bool VrCombat_NutBeltOn(void) {
    return VrCombat::WeaponAimOn() && VrItemSelect_ModeActive() && VrCombat_InPlay();
}

extern "C" bool VrCombat_NutUsesBelt(Player* player) {
    return (player != NULL) && (player->heldItemAction == PLAYER_IA_DEKU_NUT) && VrCombat_NutBeltOn();
}

extern "C" bool VrCombat_NutTakesOut(Player* player) {
    return (player != NULL) && (player->heldItemAction != PLAYER_IA_DEKU_NUT) && VrCombat_NutBeltOn() &&
           (AMMO(ITEM_NUT) > 0);
}

extern "C" bool VrCombat_NutBeltUseNow(Player* player) {
    return BeltReady(player) && VrBelt::OffHandTriggerHeld() && !VrItemSelect_BlocksUse(player);
}

extern "C" bool VrCombat_NutBeltPos(Player* player, float* outXyz) {
    if ((player == NULL) || !BeltReady(player)) {
        return false;
    }
    const VrNutThrow::Vec3 p = VrBelt::BeltPos(player, VrNutThrow::kBeltRaiseM);
    outXyz[0] = p.x;
    outXyz[1] = p.y;
    outXyz[2] = p.z;
    return true;
}

extern "C" bool VrCombat_NutHeldPos(Player* player, float* outXyz) {
    if ((player == NULL) || !VrCombat_NutUsesBelt(player) || (sHolder.Hand() < 0)) {
        return false;
    }
    return VrBelt::PosInHand(sHolder.Hand(), VrNutThrow::kPalmOffsetM, outXyz);
}

extern "C" float VrCombat_NutDrawScale(void) {
    return VrNutThrow::DrawScale(VrBelt::UnitsPerMeter());
}

extern "C" uint8_t VrCombat_NutFlightTicks(void) {
    return VrNutThrow::kFlightTicks;
}

extern "C" bool VrCombat_NutGripConsumed(int32_t vrHand, uint16_t vrBtnMask) {
    if (!(vrBtnMask & VR_BTN_GRIP) || (gPlayState == NULL)) {
        return false;
    }
    Player* player = GET_PLAYER(gPlayState);
    // Only the grip that holds the nut, or the grip of a hand at the belt.
    return VrCombat_NutUsesBelt(player) && ((sHolder.Hand() == vrHand) || sInReach[vrHand]);
}
