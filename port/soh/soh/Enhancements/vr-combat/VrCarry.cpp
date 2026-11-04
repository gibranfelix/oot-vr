extern "C" {
#include "z64.h"
#include "macros.h"
#include "functions.h"
#include "variables.h"
#include "overlays/actors/ovl_En_Ishi/z_en_ishi.h"
extern PlayState* gPlayState;
}

#include "VrCombat.h"
#include "VrBelt.h"
#include "VrCarry.h"

#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <vr_interface.h>

// The carry (#158). Read "Carry" in docs/architecture.md.

namespace {

// The offers of the last tick. The actors update after the player, so the next tick reads them.
struct Offer {
    Actor* actor;
    int16_t id;
    uint8_t category;
};
constexpr int kMaxOffers = 16;
Offer sOffers[kMaxOffers];
int sOfferCount = 0;

// The valid offers. Same index as sCandidates.
Actor* sCandidateActors[kMaxOffers];
VrCarry::Candidate sCandidates[kMaxOffers];
int sCandidateCount = 0;

VrCarry::Hold sHold;
bool sInReach[2] = { false, false };
// The object of the last grab, and its offset in the frame of the hand.
Actor* sTarget = NULL;
VrCarry::Kind sTargetKind = VrCarry::Kind::None;
VrCarry::Vec3 sGrabOffset = { 0.0f, 0.0f, 0.0f };
// The next Player_Update takes the grab. The release stays ready while Link holds the object.
bool sGrabReady = false;
bool sReleaseReady = false;
VrCombatBombRelease sRelease;

bool On() {
    return VrCombat_Active() && CVarGetInteger("gVrMotionHands", 1);
}

VrCarry::Kind KindOf(Actor* actor) {
    switch (actor->id) {
        case ACTOR_OBJ_TSUBO:
            return VrCarry::Kind::Pot;
        case ACTOR_EN_ISHI:
            // The silver boulder needs two hands (#159).
            return ((actor->params & 1) == ROCK_SMALL) ? VrCarry::Kind::SmallRock : VrCarry::Kind::None;
        case ACTOR_EN_KUSA:
            return VrCarry::Kind::Bush;
        case ACTOR_OBJ_KIBAKO:
            return VrCarry::Kind::Crate;
        case ACTOR_EN_BOMBF:
            return VrCarry::Kind::BombFlower;
        case ACTOR_EN_BOM:
            return VrCarry::Kind::Bomb;
        case ACTOR_EN_NIW:
            return VrCarry::Kind::Cucco;
        default:
            return VrCarry::Kind::None;
    }
}

VrCarry::Shape DrawnShape(VrCarry::Kind kind) {
    VrCarry::Shape s = VrCarry::ShapeOf(kind);
    if (kind == VrCarry::Kind::Bomb) {
        // EnBom_Draw draws the bomb smaller in VR first person.
        const float k = VrCombat_BombDrawScale();
        s.radius *= k;
        s.height *= k;
    }
    return s;
}

// The rule of func_8083A0F4: the bomb flower and the bush need strength.
bool StrongEnough(Actor* actor) {
    const bool weak =
        ((actor->id == ACTOR_EN_BOMBF) || (actor->id == ACTOR_EN_KUSA)) && (Player_GetStrength() <= PLAYER_STR_NONE);
    return !GameInteractor_Should(VB_PREVENT_STRENGTH, weak);
}

// The actor is still in the actor list.
bool Alive(PlayState* play, const Offer& o) {
    if (o.category >= ACTORCAT_MAX) {
        return false;
    }
    for (Actor* a = play->actorCtx.actorLists[o.category].head; a != NULL; a = a->next) {
        if (a == o.actor) {
            return (a->id == o.id) && (a->update != NULL) && (a->parent == NULL);
        }
    }
    return false;
}

void MakeCandidates(PlayState* play) {
    sCandidateCount = 0;
    for (int i = 0; i < sOfferCount; i++) {
        if (!Alive(play, sOffers[i])) {
            continue;
        }
        Actor* actor = sOffers[i].actor;
        const VrCarry::Kind kind = KindOf(actor);
        if ((kind == VrCarry::Kind::None) || !StrongEnough(actor)) {
            continue;
        }
        sCandidateActors[sCandidateCount] = actor;
        sCandidates[sCandidateCount] = { { actor->world.pos.x, actor->world.pos.y, actor->world.pos.z },
                                         DrawnShape(kind) };
        sCandidateCount++;
    }
    sOfferCount = 0;
}

// Another item uses the grip of this hand.
bool GripBlocked(int hand) {
    return VrCombat_BombGripConsumed(hand, VR_BTN_GRIP) || VrCombat_NutGripConsumed(hand, VR_BTN_GRIP) ||
           VrCombat_BoomerangGripConsumed(hand, VR_BTN_GRIP) || VrCombat_BowPullGripConsumed(hand, VR_BTN_GRIP);
}

bool CanGrab(Player* player) {
    return (player->heldActor == NULL) && !VrBelt::Blocked(player) &&
           !(player->stateFlags2 & PLAYER_STATE2_UNDERWATER);
}

// The bomb flower gives Link a new actor when he lifts it (func_8002F5C4).
void FollowNewHeldActor(Player* player) {
    if ((sTarget != NULL) && (sHold.Hand() >= 0) && (player->stateFlags1 & PLAYER_STATE1_CARRYING_ACTOR) &&
        (player->heldActor != NULL) && (player->heldActor != sTarget) &&
        (player->interactRangeActor == player->heldActor)) {
        sTarget = player->heldActor;
        sTargetKind = KindOf(sTarget);
    }
}

// Link lifts or holds the object of the last grab.
bool Carrying(Player* player) {
    if ((sTarget == NULL) || !(player->stateFlags1 & PLAYER_STATE1_CARRYING_ACTOR)) {
        return false;
    }
    return (player->heldActor == sTarget) || ((player->heldActor == NULL) && (player->interactRangeActor == sTarget));
}

void Feed() {
    for (int hand = 0; hand < 2; hand++) {
        const VrCombat::TickPath& path = VrCombat::GetTickPath(hand);
        if (path.count == 0) {
            sHold.Clear(hand);
        }
        for (int i = 0; i < path.count; i++) {
            const VrHandSample& s = path.samples[i];
            sHold.AddSample(hand, { { s.pos[0], s.pos[1], s.pos[2] },
                                    { s.linVelMps[0], s.linVelMps[1], s.linVelMps[2] },
                                    s.timeNs });
        }
    }
}

VrCarry::Input MakeInput(Player* player, bool on) {
    VrCarry::Input in;
    in.canGrab = on && CanGrab(player);
    in.carrying = Carrying(player);
    in.firstHand = VrBelt::SwordHand();
    const float margin = VrCarry::kGrabMarginM * VrBelt::UnitsPerMeter();
    for (int hand = 0; hand < 2; hand++) {
        float pos[3];
        float quat[4];
        VrCarry::HandInput& h = in.hands[hand];
        h.gripHeld = (VR_GetControllerButton(hand) & VR_BTN_GRIP) != 0;
        h.blocked = GripBlocked(hand);
        if (VR_GetHandPose(hand, pos, quat)) {
            h.pos = { pos[0], pos[1], pos[2] };
            h.target = VrCarry::PickTarget(h.pos, sCandidates, sCandidateCount, margin);
        } else {
            h.pos = { 1.0e6f, 1.0e6f, 1.0e6f };
            h.target = -1;
        }
    }
    return in;
}

// False when the hand is not tracked.
bool PosInHand(int hand, float* outXyz) {
    float pos[3];
    float quat[4];
    if ((hand < 0) || !VR_GetHandPose(hand, pos, quat)) {
        return false;
    }
    const VrCarry::Vec3 p = VrCarry::HeldPos({ pos[0], pos[1], pos[2] }, quat, sGrabOffset);
    outXyz[0] = p.x;
    outXyz[1] = p.y;
    outXyz[2] = p.z;
    return true;
}

void Grab(int hand, int target) {
    float pos[3];
    float quat[4];
    Actor* actor = sCandidateActors[target];
    sTarget = actor;
    sTargetKind = KindOf(actor);
    sGrabOffset = { 0.0f, 0.0f, 0.0f };
    if (VR_GetHandPose(hand, pos, quat)) {
        sGrabOffset = VrCarry::GrabOffset({ pos[0], pos[1], pos[2] }, quat,
                                          { actor->world.pos.x, actor->world.pos.y, actor->world.pos.z });
    }
    sGrabReady = true;
}

void MakeRelease(Player* player, const VrCarry::Result& r) {
    if (!PosInHand(r.hand, sRelease.pos)) {
        sRelease.pos[0] = r.pos.x + sGrabOffset.x;
        sRelease.pos[1] = r.pos.y + sGrabOffset.y;
        sRelease.pos[2] = r.pos.z + sGrabOffset.z;
    }
    sRelease.thrown = (r.event == VrCarry::Event::Throw);
    VrCarry::Vec3 v = { 0.0f, 0.0f, 0.0f };
    if (!sRelease.thrown) {
        Vec3f pos = { sRelease.pos[0], sRelease.pos[1] + 1.0f, sRelease.pos[2] };
        CollisionPoly* poly;
        s32 bgId;
        const f32 floorY = BgCheck_EntityRaycastFloor3(&gPlayState->colCtx, &poly, &bgId, &pos);
        const float speed =
            (poly != NULL) ? VrCarry::DropSpeed(sRelease.pos[1] - floorY, VrBelt::UnitsPerMeter()) : VrCarry::kFallSpeed;
        v.x = Math_SinS(player->actor.shape.rot.y) * speed;
        v.z = Math_CosS(player->actor.shape.rot.y) * speed;
    } else {
        v = VrCarry::ThrowVelocity(r.velMps, 60.0f / R_UPDATE_RATE, DrawnShape(sTargetKind).gravity);
        // The hand velocity does not contain the walk of Link.
        v.x += player->actor.velocity.x;
        v.z += player->actor.velocity.z;
    }
    sRelease.velocity[0] = v.x;
    sRelease.velocity[1] = v.y;
    sRelease.velocity[2] = v.z;
    sReleaseReady = true;
}

void Haptics(const VrCarry::Result& r) {
    for (int hand = 0; hand < 2; hand++) {
        sInReach[hand] = r.inReach[hand];
        if (r.reachPulse[hand]) {
            VR_TriggerHaptic(hand, 0.25f, 0.0f, 15.0f);
        }
    }
    if (r.event == VrCarry::Event::Grab) {
        VR_TriggerHaptic(r.hand, 0.5f, 0.0f, 35.0f);
    } else if (r.event == VrCarry::Event::Throw) {
        VR_TriggerHaptic(r.hand, 0.35f, 0.0f, 25.0f);
    }
}

} // namespace

void VrCombat::Carry_OnPlayerUpdate(Player* player) {
    sGrabReady = false;
    if (!VR_IsInitialized() || (player == NULL) || (gPlayState == NULL)) {
        sReleaseReady = false;
        sOfferCount = 0;
        sCandidateCount = 0;
        return;
    }
    const bool on = On();
    MakeCandidates(gPlayState);
    Feed();
    FollowNewHeldActor(player);

    const VrCarry::Input in = MakeInput(player, on);
    if (!in.carrying) {
        sReleaseReady = false;
        if (sHold.Hand() < 0) {
            sTarget = NULL;
        }
    }
    const VrCarry::Result r = sHold.Update(in);
    Haptics(r);
    switch (r.event) {
        case VrCarry::Event::Grab:
            Grab(r.hand, r.target);
            break;
        case VrCarry::Event::Throw:
        case VrCarry::Event::Drop:
            MakeRelease(player, r);
            break;
        case VrCarry::Event::None:
            break;
    }
}

extern "C" void VrCombat_CarryOffer(Actor* actor) {
    if ((actor == NULL) || !On() || (sOfferCount >= kMaxOffers)) {
        return;
    }
    for (int i = 0; i < sOfferCount; i++) {
        if (sOffers[i].actor == actor) {
            return;
        }
    }
    sOffers[sOfferCount++] = { actor, actor->id, actor->category };
}

extern "C" Actor* VrCombat_CarryTakeGrab(void) {
    if (!sGrabReady || (sTarget == NULL)) {
        return NULL;
    }
    sGrabReady = false;
    return sTarget;
}

extern "C" bool VrCombat_CarryHeldPos(Player* player, float* outXyz) {
    if (player != NULL) {
        FollowNewHeldActor(player);
    }
    if ((player == NULL) || (sTarget == NULL) || (player->heldActor != sTarget)) {
        return false;
    }
    if (sReleaseReady) {
        outXyz[0] = sRelease.pos[0];
        outXyz[1] = sRelease.pos[1];
        outXyz[2] = sRelease.pos[2];
        return true;
    }
    return PosInHand(sHold.Hand(), outXyz);
}

extern "C" bool VrCombat_CarryTakeRelease(Player* player, VrCombatBombRelease* out) {
    if (!sReleaseReady || (player == NULL) || (sTarget == NULL) || (player->heldActor != sTarget)) {
        return false;
    }
    sReleaseReady = false;
    sTarget = NULL;
    *out = sRelease;
    return true;
}

extern "C" bool VrCombat_CarryOwnsGrip(void) {
    return VR_IsInitialized() && (sHold.Hand() >= 0);
}

extern "C" bool VrCombat_CarryGripConsumed(int32_t vrHand, uint16_t vrBtnMask) {
    if (!(vrBtnMask & VR_BTN_GRIP) || !VR_IsInitialized() || (vrHand < 0) || (vrHand > 1)) {
        return false;
    }
    return (sHold.Hand() == vrHand) || sInReach[vrHand];
}
