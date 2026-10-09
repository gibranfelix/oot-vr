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

// The bomb belt (#97), also for the bombchu (#98). Read "Bomb belt" in docs/architecture.md.

namespace {

VrBombThrow::Holder sHolder;
bool sInReach[2] = { false, false };
// The grab is ready for one tick: the next Player_Update takes it. The release stays ready while
// Link carries the bomb.
bool sGrabReady = false;
bool sReleaseReady = false;
VrCombatBombRelease sRelease;
// The release of a bombchu. EnBomChu_WaitForRelease takes it one time.
struct BombchuStart {
    Actor* chu = NULL;
    Vec3f handPos = { 0.0f, 0.0f, 0.0f };
    int16_t yaw = 0;
    bool freeLastTick = false; // Link did not carry the bombchu in the last tick.
};
BombchuStart sChuStart;
// The height of the low wall test above the floor, in game units.
constexpr float kBombchuLowLineY = 10.0f;
// A new bomb comes to the belt this number of ticks after the last bomb left the hand.
constexpr int kRefillTicks = 10;
int sRefillTicks = kRefillTicks;

bool IsBombchu(Player* player) {
    return player->heldItemAction == PLAYER_IA_BOMBCHU;
}

// The EnBom and EnBomChu positions are at the bottom of the model. This is the height of the center.
float CenterHeight(Player* player) {
    const float h = IsBombchu(player) ? VrBombThrow::kBombchuCenterHeight : 0.5f * VrBombThrow::kBombModelDiameter;
    return h * VrCombat_BombDrawScale();
}

// Link carries a bomb or a bombchu (from the belt, or from the vanilla take-out).
bool Carrying(Player* player) {
    if (!(player->stateFlags1 & PLAYER_STATE1_CARRYING_ACTOR) || (player->heldActor == NULL)) {
        return false;
    }
    return ((player->heldActor->id == ACTOR_EN_BOM) && (player->heldItemAction == PLAYER_IA_BOMB)) ||
           ((player->heldActor->id == ACTOR_EN_BOM_CHU) && IsBombchu(player));
}

bool ExplosiveLimit(PlayState* play) {
    return (play->actorCtx.actorLists[ACTORCAT_EXPLOSIVE].length >= 3) &&
           !CVarGetInteger(CVAR_ENHANCEMENT("RemoveExplosiveLimit"), 0);
}

// The ammo checks of Player_UseItem.
bool HasAmmo(PlayState* play, Player* player) {
    if (play->bombchuBowlingStatus != 0) {
        return play->bombchuBowlingStatus > 0;
    }
    return (AMMO(IsBombchu(player) ? ITEM_BOMBCHU : ITEM_BOMB) > 0) && !ExplosiveLimit(play);
}

bool BeltReady(Player* player) {
    return (gPlayState != NULL) && VrCombat_BombUsesBelt(player) && (player->heldActor == NULL) &&
           !VrBelt::Blocked(player) && HasAmmo(gPlayState, player) && (sRefillTicks == 0);
}

// The actor position for a bomb or a bombchu in the hand. The center is in front of the palm.
bool BombPosInHand(Player* player, int hand, float* outXyz) {
    if (!VrBelt::PosInHand(hand, VrBombThrow::kPalmOffsetM, outXyz)) {
        return false;
    }
    outXyz[1] -= CenterHeight(player);
    return true;
}

// The run direction of the bombchu. False when there is no direction on the floor plane.
bool BombchuYaw(int hand, const VrBombThrow::Vec3& velMps, int16_t* outYaw) {
    float aimPos[3];
    float aimDir[3] = { 0.0f, 0.0f, 0.0f };
    VR_GetAimRay(hand, aimPos, aimDir);
    float yaw;
    if (!VrBombThrow::BombchuRunYaw(velMps, { aimDir[0], aimDir[1], aimDir[2] }, &yaw)) {
        return false;
    }
    *outYaw = (int16_t)(int32_t)(yaw * (0x8000 / M_PI));
    return true;
}

void MakeRelease(Player* player, const VrBombThrow::Result& r) {
    if (!BombPosInHand(player, r.hand, sRelease.pos)) {
        sRelease.pos[0] = r.pos.x;
        sRelease.pos[1] = r.pos.y - CenterHeight(player);
        sRelease.pos[2] = r.pos.z;
    }
    if (IsBombchu(player)) {
        // The bombchu does not fly. EnBomChu_WaitForRelease puts it on the floor.
        sChuStart.chu = player->heldActor;
        sChuStart.handPos = { sRelease.pos[0], sRelease.pos[1], sRelease.pos[2] };
        if (!BombchuYaw(r.hand, r.velMps, &sChuStart.yaw)) {
            sChuStart.yaw = player->actor.shape.rot.y;
        }
        sRelease.thrown = false;
        sRelease.velocity[0] = sRelease.velocity[1] = sRelease.velocity[2] = 0.0f;
        sReleaseReady = true;
        return;
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
        // This hook runs at the end of Player_Update. EnBomChu updates after the player, thus it
        // takes the start in the tick of the release.
        if (sChuStart.freeLastTick) {
            sChuStart.chu = NULL;
        }
        sChuStart.freeLastTick = true;
    } else {
        sChuStart.freeLastTick = false;
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
    return (player != NULL) &&
           ((player->heldItemAction == PLAYER_IA_BOMB) || (player->heldItemAction == PLAYER_IA_BOMBCHU)) &&
           VrCombat::WeaponAimOn() && VrItemSelect_ModeActive() && VrCombat_InPlay();
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
    return (sHolder.Hand() >= 0) && BombPosInHand(player, sHolder.Hand(), outXyz);
}

extern "C" bool VrCombat_BombchuHeldYaw(Player* player, int16_t* outYaw) {
    if ((player == NULL) || !VrCombat_BombUsesBelt(player) || !IsBombchu(player) || !Carrying(player) ||
        (sHolder.Hand() < 0)) {
        return false;
    }
    if (sReleaseReady && (sChuStart.chu == player->heldActor)) {
        *outYaw = sChuStart.yaw;
        return true;
    }
    return BombchuYaw(sHolder.Hand(), { 0.0f, 0.0f, 0.0f }, outYaw);
}

extern "C" bool VrCombat_BombchuTakeStart(Actor* chu, float* outXyz, int16_t* outYaw) {
    if ((chu == NULL) || (chu != sChuStart.chu) || (gPlayState == NULL)) {
        return false;
    }
    sChuStart.chu = NULL;
    Player* player = GET_PLAYER(gPlayState);
    *outYaw = sChuStart.yaw;
    outXyz[0] = player->actor.world.pos.x;
    outXyz[1] = player->actor.world.pos.y;
    outXyz[2] = player->actor.world.pos.z;

    // The floor below the hand. Else the position of Link, as in the vanilla game. The second line
    // test finds a low wall.
    Vec3f hand = sChuStart.handPos;
    CollisionPoly* poly;
    s32 bgId;
    auto wall = [&](Vec3f from, Vec3f to) {
        Vec3f hit;
        return BgCheck_EntityLineTest1(&gPlayState->colCtx, &from, &to, &hit, &poly, true, true, true, true, &bgId);
    };
    if (wall(player->actor.focus.pos, hand)) {
        return true;
    }
    const f32 floorY = BgCheck_EntityRaycastFloor3(&gPlayState->colCtx, &poly, &bgId, &hand);
    if ((poly == NULL) || !VrBombThrow::BombchuFloorOk(floorY, player->actor.world.pos.y)) {
        return true;
    }
    const Vec3f linkLow = { player->actor.world.pos.x, player->actor.world.pos.y + kBombchuLowLineY,
                            player->actor.world.pos.z };
    if (wall(linkLow, { hand.x, floorY + kBombchuLowLineY, hand.z })) {
        return true;
    }
    outXyz[0] = hand.x;
    outXyz[1] = floorY;
    outXyz[2] = hand.z;
    return true;
}

extern "C" float VrCombat_BombDrawScale(void) {
    if (!VR_IsInitialized() || !VR_GetFirstPerson()) {
        return 1.0f;
    }
    return VrBombThrow::DrawScale(VrBelt::UnitsPerMeter());
}

extern "C" float VrCombat_BombCenterHeight(Player* player) {
    return CenterHeight(player);
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
