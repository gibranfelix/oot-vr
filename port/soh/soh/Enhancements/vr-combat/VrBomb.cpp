extern "C" {
#include "z64.h"
#include "macros.h"
#include "functions.h"
#include "variables.h"
extern PlayState* gPlayState;
}

#include "VrCombat.h"
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

// Link cannot take a bomb in these states.
constexpr uint32_t kNoBeltFlags =
    PLAYER_STATE1_LOADING | PLAYER_STATE1_INPUT_DISABLED | PLAYER_STATE1_TALKING | PLAYER_STATE1_DEAD |
    PLAYER_STATE1_START_CHANGING_HELD_ITEM | PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_CARRYING_ACTOR |
    PLAYER_STATE1_HANGING_OFF_LEDGE | PLAYER_STATE1_CLIMBING_LEDGE | PLAYER_STATE1_FIRST_PERSON |
    PLAYER_STATE1_CLIMBING_LADDER | PLAYER_STATE1_ON_HORSE | PLAYER_STATE1_DAMAGED | PLAYER_STATE1_IN_WATER |
    PLAYER_STATE1_IN_ITEM_CS | PLAYER_STATE1_IN_CUTSCENE;

int SwordHand() {
    return CVarGetInteger("gVrLeftHanded", 0) ? VR_HAND_LEFT : VR_HAND_RIGHT;
}

bool OffHandTriggerHeld() {
    return (VR_GetControllerButton(SwordHand() ^ 1) & VR_BTN_TRIGGER) != 0;
}

float UnitsPerMeter() {
    const float ws = VR_GetWorldScale();
    return (ws < 1.0f) ? 35.0f : ws;
}

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
           !(player->stateFlags1 & kNoBeltFlags) && (AMMO(ITEM_BOMB) > 0) && !ExplosiveLimit(gPlayState) &&
           (sRefillTicks == 0);
}

VrBombThrow::Vec3 BeltPosNow(Player* player) {
    float eye[3];
    float fwd[3];
    float up[3];
    VR_GetCameraPose(eye, fwd, up);
    const float yaw = player->actor.shape.rot.y * (M_PI / 0x8000);
    return VrBombThrow::BeltAnchor({ eye[0], eye[1], eye[2] }, yaw, UnitsPerMeter());
}

VrBombThrow::HandInput Hand(int hand) {
    float pos[3];
    float quat[4];
    VrBombThrow::HandInput h;
    h.gripHeld = (VR_GetControllerButton(hand) & VR_BTN_GRIP) != 0;
    // An untracked hand is far from the belt.
    h.pos = VR_GetHandPose(hand, pos, quat) ? VrBombThrow::Vec3{ pos[0], pos[1], pos[2] }
                                             : VrBombThrow::Vec3{ 1.0e6f, 1.0e6f, 1.0e6f };
    return h;
}

void MakeRelease(Player* player, const VrBombThrow::Result& r) {
    sRelease.pos[0] = r.pos.x;
    sRelease.pos[1] = r.pos.y - CenterHeight();
    sRelease.pos[2] = r.pos.z;
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
    for (int hand = 0; hand < 2; hand++) {
        const TickPath& path = GetTickPath(hand);
        if (path.count == 0) {
            sHolder.Clear(hand);
        }
        for (int i = 0; i < path.count; i++) {
            const VrHandSample& s = path.samples[i];
            sHolder.AddSample(hand, { { s.pos[0], s.pos[1], s.pos[2] },
                                      { s.linVelMps[0], s.linVelMps[1], s.linVelMps[2] },
                                      s.timeNs });
        }
    }

    const bool carrying = Carrying(player);
    if (!carrying) {
        sReleaseReady = false;
    }
    if (carrying || !VrCombat_BombUsesBelt(player)) {
        sRefillTicks = kRefillTicks;
    } else if (sRefillTicks > 0) {
        sRefillTicks--;
    }

    VrBombThrow::Input in;
    in.beltReady = BeltReady(player);
    in.carrying = carrying;
    in.swapChord = VrItemSelect_SwapChordHeld();
    in.hands[VR_HAND_LEFT] = Hand(VR_HAND_LEFT);
    in.hands[VR_HAND_RIGHT] = Hand(VR_HAND_RIGHT);
    in.beltPos = BeltPosNow(player);
    in.grabRadius = VrBombThrow::kGrabRadiusM * UnitsPerMeter();
    in.firstHand = SwordHand();
    const VrBombThrow::Result r = sHolder.Update(in);

    for (int hand = 0; hand < 2; hand++) {
        sInReach[hand] = r.inReach[hand];
        if (r.reachPulse[hand]) {
            VR_TriggerHaptic(hand, 0.25f, 0.0f, 15.0f);
        }
    }
    switch (r.event) {
        case VrBombThrow::Event::Grab:
            sGrabReady = true;
            VR_TriggerHaptic(r.hand, 0.5f, 0.0f, 35.0f);
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
    return BeltReady(player) && OffHandTriggerHeld() && !VrItemSelect_BlocksUse(player);
}

extern "C" bool VrCombat_BombBeltPos(Player* player, float* outXyz) {
    if ((player == NULL) || !BeltReady(player)) {
        return false;
    }
    const VrBombThrow::Vec3 p = BeltPosNow(player);
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
    float quat[4];
    if ((sHolder.Hand() < 0) || !VR_GetHandPose(sHolder.Hand(), outXyz, quat)) {
        return false;
    }
    outXyz[1] -= CenterHeight();
    return true;
}

extern "C" bool VrCombat_BombInHand(Player* player, bool* swordHand) {
    if ((player == NULL) || !VrCombat_BombUsesBelt(player) || !Carrying(player) || (sHolder.Hand() < 0)) {
        return false;
    }
    *swordHand = (sHolder.Hand() == SwordHand());
    return true;
}

extern "C" float VrCombat_BombDrawScale(void) {
    if (!VR_IsInitialized() || !VR_GetFirstPerson()) {
        return 1.0f;
    }
    return VrBombThrow::DrawScale(UnitsPerMeter());
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
            (!OffHandTriggerHeld() || VrItemSelect_BlocksUse(player))) {
            *should = false;
        }
    });
}

static RegisterShipInitFunc initVrBomb(RegisterVrBomb, {});
