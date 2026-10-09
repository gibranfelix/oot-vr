extern "C" {
#include "z64.h"
#include "macros.h"
#include "functions.h"
#include "variables.h"
extern PlayState* gPlayState;
}

#include "VrCombat.h"
#include "VrBelt.h"
#include "VrBombThrow.h"

#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ShipInit.hpp"
#include "soh/cvar_prefixes.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <vr_interface.h>

// The bomb belt (#97). Read "Bomb belt" in docs/architecture.md.

namespace {

VrBombThrow::Holder sHolder;
bool sInReach[2] = { false, false };
// The grab is ready for one tick: the next Player_Update takes it. The release stays ready while
// Link carries the bomb.
bool sGrabReady = false;
bool sReleaseReady = false;
VrCombatBombRelease sRelease;
// A new bomb comes to the belt this number of ticks after the last bomb left the hand.
constexpr int kRefillTicks = 10;
int sRefillTicks = kRefillTicks;

// The EnBom position is the bottom of the bomb. This is the height of the center.
float CenterHeight() {
    return 0.5f * VrBombThrow::kBombModelDiameter * VrCombat_BombDrawScale();
}

// Link carries a bomb (from the belt, or from the vanilla take-out).
bool Carrying(Player* player) {
    return (player->stateFlags1 & PLAYER_STATE1_CARRYING_ACTOR) && (player->heldActor != NULL) &&
           (player->heldActor->id == ACTOR_EN_BOM) && (player->heldItemAction == PLAYER_IA_BOMB);
}

bool ExplosiveLimit(PlayState* play) {
    return (play->actorCtx.actorLists[ACTORCAT_EXPLOSIVE].length >= 3) &&
           !CVarGetInteger(CVAR_ENHANCEMENT("RemoveExplosiveLimit"), 0);
}

bool BeltReady(Player* player) {
    return (gPlayState != NULL) && VrCombat_BombUsesBelt(player) && (player->heldActor == NULL) &&
           !VrBelt::Blocked(player) && (AMMO(ITEM_BOMB) > 0) && !ExplosiveLimit(gPlayState) && (sRefillTicks == 0);
}

// The EnBom position for a bomb in the hand: the bomb center is in front of the palm.
bool BombPosInHand(int hand, float* outXyz) {
    if (!VrBelt::PosInHand(hand, VrBombThrow::kPalmOffsetM, outXyz)) {
        return false;
    }
    outXyz[1] -= CenterHeight();
    return true;
}

void MakeRelease(Player* player, const VrBombThrow::Result& r) {
    if (!BombPosInHand(r.hand, sRelease.pos)) {
        sRelease.pos[0] = r.pos.x;
        sRelease.pos[1] = r.pos.y - CenterHeight();
        sRelease.pos[2] = r.pos.z;
    }
    sRelease.thrown = (r.event == VrBombThrow::Event::Throw);
    VrBombThrow::Vec3 v = { 0.0f, 0.0f, 0.0f };
    if (sRelease.thrown) {
        v = VrBombThrow::ThrowVelocity(r.velMps, 60.0f / R_UPDATE_RATE);
        // The hand velocity does not contain the walk of Link.
        v.x += player->actor.velocity.x;
        v.z += player->actor.velocity.z;
    }
    sRelease.velocity[0] = v.x;
    sRelease.velocity[1] = v.y;
    sRelease.velocity[2] = v.z;
    sReleaseReady = true;
}

} // namespace

void VrCombat::Bomb_OnPlayerUpdate(Player* player) {
    sGrabReady = false;
    if (!VR_IsInitialized() || (player == NULL)) {
        sReleaseReady = false;
        return;
    }
    VrBelt::Feed(sHolder);

    const bool carrying = Carrying(player);
    if (!carrying) {
        sReleaseReady = false;
    }
    if (carrying || !VrCombat_BombUsesBelt(player)) {
        sRefillTicks = kRefillTicks;
    } else if (sRefillTicks > 0) {
        sRefillTicks--;
    }

    const VrBombThrow::Result r = sHolder.Update(VrBelt::MakeInput(player, BeltReady(player), carrying));
    VrBelt::Haptics(r, sInReach);
    switch (r.event) {
        case VrBombThrow::Event::Grab:
            sGrabReady = true;
            break;
        case VrBombThrow::Event::Throw:
            MakeRelease(player, r);
            VR_TriggerHaptic(r.hand, 0.35f, 0.0f, 25.0f);
            break;
        case VrBombThrow::Event::Drop:
            MakeRelease(player, r);
            break;
        case VrBombThrow::Event::None:
            break;
    }
}

extern "C" bool VrCombat_BombUsesBelt(Player* player) {
    return (player != NULL) && (player->heldItemAction == PLAYER_IA_BOMB) && VrCombat::WeaponAimOn() &&
           VrItemSelect_ModeActive() && VrCombat_InPlay();
}

extern "C" bool VrCombat_BombBeltUseNow(Player* player) {
    return BeltReady(player) && VrBelt::OffHandTriggerHeld() && !VrItemSelect_BlocksUse(player);
}

extern "C" bool VrCombat_BombBeltPos(Player* player, float* outXyz) {
    if ((player == NULL) || !BeltReady(player)) {
        return false;
    }
    const VrBombThrow::Vec3 p = VrBelt::BeltPos(player);
    outXyz[0] = p.x;
    outXyz[1] = p.y;
    outXyz[2] = p.z;
    return true;
}

extern "C" bool VrCombat_BombTakeGrab(void) {
    const bool ready = sGrabReady;
    sGrabReady = false;
    return ready;
}

extern "C" bool VrCombat_BombHeldPos(Player* player, float* outXyz) {
    if ((player == NULL) || !VrCombat_BombUsesBelt(player) || !Carrying(player)) {
        return false;
    }
    if (sReleaseReady) {
        outXyz[0] = sRelease.pos[0];
        outXyz[1] = sRelease.pos[1];
        outXyz[2] = sRelease.pos[2];
        return true;
    }
    return (sHolder.Hand() >= 0) && BombPosInHand(sHolder.Hand(), outXyz);
}

extern "C" float VrCombat_BombDrawScale(void) {
    if (!VR_IsInitialized() || !VR_GetFirstPerson()) {
        return 1.0f;
    }
    return VrBombThrow::DrawScale(VrBelt::UnitsPerMeter());
}

extern "C" bool VrCombat_BombTakeRelease(VrCombatBombRelease* out) {
    if (!sReleaseReady) {
        return false;
    }
    sReleaseReady = false;
    *out = sRelease;
    return true;
}

extern "C" bool VrCombat_BombGripConsumed(int32_t vrHand, uint16_t vrBtnMask) {
    if (!(vrBtnMask & VR_BTN_GRIP) || (gPlayState == NULL)) {
        return false;
    }
    Player* player = GET_PLAYER(gPlayState);
    // Only the grip that holds the bomb, or the grip of a hand at the belt.
    return VrCombat_BombUsesBelt(player) && ((sHolder.Hand() == vrHand) || sInReach[vrHand]);
}

static void RegisterVrBomb() {
    // A, B, and the C buttons do not throw the bomb in the grip. The off-hand trigger throws it.
    COND_VB_SHOULD(VB_THROW_OR_PUT_DOWN_HELD_ITEM, true, {
        Player* player = (gPlayState != NULL) ? GET_PLAYER(gPlayState) : NULL;
        if (VrCombat_BombUsesBelt(player) && Carrying(player) && (sHolder.Hand() >= 0) &&
            (!VrBelt::OffHandTriggerHeld() || VrItemSelect_BlocksUse(player))) {
            *should = false;
        }
    });
}

static RegisterShipInitFunc initVrBomb(RegisterVrBomb, {});
