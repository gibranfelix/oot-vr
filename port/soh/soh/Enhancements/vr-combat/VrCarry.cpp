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

// Read "Carry" in docs/architecture.md.

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

// The grabbable wall of an actor that Link touches in this tick: a block.
struct WallOffer {
    Actor* actor;
    float n[3];
    float d;
};
WallOffer sWall = { NULL, { 0.0f, 0.0f, 0.0f }, 0.0f };

// The valid offers, then the block wall. The wall has no cylinder.
constexpr int kMaxCandidates = kMaxOffers + 1;
Actor* sCandidateActors[kMaxCandidates];
VrCarry::Kind sCandidateKinds[kMaxCandidates];
VrCarry::Candidate sCandidates[kMaxCandidates];
int sCylinderCount = 0;
int sCandidateCount = 0;
int sBlockIndex = -1;

VrCarry::Hold sHold;
bool sInReach[2] = { false, false };
// The object of the last grab, and its offset in the frame of the hand.
Actor* sTarget = NULL;
VrCarry::Kind sTargetKind = VrCarry::Kind::None;
VrCarry::Vec3 sGrabOffset = { 0.0f, 0.0f, 0.0f };
// The grips that took a wall stay reserved until one opens.
bool sWallGrips = false;
// Block push (#160): the wall normal at the grab, and the stick of the hands.
float sPushN[3] = { 0.0f, 0.0f, 0.0f };
VrCarry::PushStick sPushStick;
int sPushDir = 0;
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
            return ((actor->params & 1) == ROCK_SMALL) ? VrCarry::Kind::SmallRock : VrCarry::Kind::SilverBoulder;
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

VrCarry::Kind WallKindOf(Actor* actor) {
    // Only the A button lifts the golden gauntlet pillar.
    return (actor->id == ACTOR_BG_HEAVY_BLOCK) ? VrCarry::Kind::None : VrCarry::Kind::Block;
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

// The strength rules of the A button.
bool StrongEnough(Actor* actor, VrCarry::Kind kind) {
    if (kind == VrCarry::Kind::SilverBoulder) {
        return Player_GetStrength() >= PLAYER_STR_SILVER_G;
    }
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

// The hands and the eyes use the same camera anchor. Link moves one tick before the anchor. Thus a
// distance from Link changes while Link pushes, and a distance from the eyes does not.
VrCarry::Vec3 EyePos() {
    float eye[3];
    float fwd[3];
    float up[3];
    VR_GetCameraPose(eye, fwd, up);
    return { eye[0], eye[1], eye[2] };
}

void AddCandidate(Actor* actor, VrCarry::Kind kind) {
    sCandidateActors[sCandidateCount] = actor;
    sCandidateKinds[sCandidateCount] = kind;
    sCandidates[sCandidateCount] = { { actor->world.pos.x, actor->world.pos.y, actor->world.pos.z },
                                     DrawnShape(kind) };
    sCandidateCount++;
}

void MakeCandidates(PlayState* play) {
    sCandidateCount = 0;
    for (int i = 0; i < sOfferCount; i++) {
        if (!Alive(play, sOffers[i])) {
            continue;
        }
        Actor* actor = sOffers[i].actor;
        const VrCarry::Kind kind = KindOf(actor);
        if ((kind == VrCarry::Kind::None) || !StrongEnough(actor, kind)) {
            continue;
        }
        AddCandidate(actor, kind);
    }
    sOfferCount = 0;
    sCylinderCount = sCandidateCount;
    sBlockIndex = -1;
    if ((sWall.actor != NULL) && (sWall.actor->update != NULL) &&
        (WallKindOf(sWall.actor) == VrCarry::Kind::Block)) {
        sBlockIndex = sCandidateCount;
        AddCandidate(sWall.actor, VrCarry::Kind::Block);
    }
    sWall.actor = NULL;
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
    const float upm = VrBelt::UnitsPerMeter();
    const float margin = VrCarry::kGrabMarginM * upm;
    const VrCarry::Vec3 link = { player->actor.world.pos.x, player->actor.world.pos.y, player->actor.world.pos.z };
    for (int hand = 0; hand < 2; hand++) {
        float pos[3];
        float quat[4];
        VrCarry::HandInput& h = in.hands[hand];
        h.gripHeld = (VR_GetControllerButton(hand) & VR_BTN_GRIP) != 0;
        h.blocked = GripBlocked(hand);
        if (VR_GetHandPose(hand, pos, quat)) {
            h.pos = { pos[0], pos[1], pos[2] };
            if ((sBlockIndex >= 0) &&
                       VrCarry::WallInReach(h.pos, link, sWall.n, sWall.d, margin, VrCarry::kWallReachM * upm)) {
                h.target = sBlockIndex;
            } else {
                h.target = VrCarry::PickTarget(h.pos, sCandidates, sCylinderCount, margin);
            }
        } else {
            h.pos = { 1.0e6f, 1.0e6f, 1.0e6f };
            h.target = -1;
        }
        h.twoHands = (h.target >= 0) && VrCarry::NeedsTwoHands(sCandidateKinds[h.target]);
    }
    return in;
}

// The hand, or the midpoint of two hands (no rotation). False when a hand is not tracked.
bool HoldPose(bool twoHands, int hand, VrCarry::Vec3* outPos, float outQuat[4]) {
    float pos[2][3];
    float quat[4];
    if (!twoHands) {
        if ((hand < 0) || !VR_GetHandPose(hand, pos[0], outQuat)) {
            return false;
        }
        *outPos = { pos[0][0], pos[0][1], pos[0][2] };
        return true;
    }
    if (!VR_GetHandPose(VR_HAND_LEFT, pos[0], quat) || !VR_GetHandPose(VR_HAND_RIGHT, pos[1], quat)) {
        return false;
    }
    *outPos = VrCarry::Mid({ pos[0][0], pos[0][1], pos[0][2] }, { pos[1][0], pos[1][1], pos[1][2] });
    outQuat[0] = outQuat[1] = outQuat[2] = 0.0f;
    outQuat[3] = 1.0f;
    return true;
}

// False when a hand is not tracked.
bool HeldPosNow(float* outXyz) {
    VrCarry::Vec3 pos;
    float quat[4];
    if (!HoldPose(sHold.TwoHands(), sHold.Hand(), &pos, quat)) {
        return false;
    }
    const VrCarry::Vec3 p = VrCarry::HeldPos(pos, quat, sGrabOffset);
    outXyz[0] = p.x;
    outXyz[1] = p.y;
    outXyz[2] = p.z;
    return true;
}

void Grab(const VrCarry::Result& r) {
    VrCarry::Vec3 pos;
    float quat[4];
    Actor* actor = sCandidateActors[r.target];
    const bool tracked = HoldPose(r.twoHands, r.hand, &pos, quat);
    sTarget = actor;
    sTargetKind = sCandidateKinds[r.target];
    sGrabOffset = { 0.0f, 0.0f, 0.0f };
    if (tracked) {
        VrCarry::Vec3 actorPos = { actor->world.pos.x, actor->world.pos.y, actor->world.pos.z };
        if (!r.twoHands) {
            actorPos = VrCarry::SnapToHand(pos, actorPos, sCandidates[r.target].shape,
                                           VrCarry::kSnapDepthM * VrBelt::UnitsPerMeter());
        }
        sGrabOffset = VrCarry::GrabOffset(pos, quat, actorPos);
    }
    if (sTargetKind == VrCarry::Kind::Block) {
        // The game moves the object. VrCombat_CarryTakeWallGrab starts the push.
        sHold.Forget();
        if (!tracked) {
            return;
        }
        for (int i = 0; i < 3; i++) {
            sPushN[i] = sWall.n[i];
        }
        sPushStick.Start(VrCarry::PushAlong(pos, EyePos(), sPushN));
    }
    sGrabReady = true;
}

void MakeRelease(Player* player, const VrCarry::Result& r) {
    if (!HeldPosNow(sRelease.pos)) {
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
    for (int hand = 0; hand < 2; hand++) {
        if (!r.twoHands && (hand != r.hand)) {
            continue;
        }
        if (r.event == VrCarry::Event::Grab) {
            VR_TriggerHaptic(hand, 0.5f, 0.0f, 35.0f);
        } else if (r.event == VrCarry::Event::Throw) {
            VR_TriggerHaptic(hand, 0.35f, 0.0f, 25.0f);
        }
    }
}

} // namespace

void VrCombat::Carry_OnPlayerUpdate(Player* player) {
    sGrabReady = false;
    if (!VR_IsInitialized() || (player == NULL) || (gPlayState == NULL)) {
        sReleaseReady = false;
        sOfferCount = 0;
        sCandidateCount = 0;
        sWall.actor = NULL;
        return;
    }
    const bool on = On();
    MakeCandidates(gPlayState);
    Feed();
    FollowNewHeldActor(player);

    const VrCarry::Input in = MakeInput(player, on);
    if (!in.hands[0].gripHeld || !in.hands[1].gripHeld) {
        sWallGrips = false;
    }
    sPushDir = 0;
    if (sWallGrips && (sTargetKind == VrCarry::Kind::Block)) {
        VrCarry::Vec3 pos;
        float quat[4];
        if (HoldPose(true, -1, &pos, quat)) {
            sPushDir = sPushStick.Update(VrCarry::PushAlong(pos, EyePos(), sPushN), VrBelt::UnitsPerMeter());
        }
    }
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
            Grab(r);
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

extern "C" void VrCombat_CarryOfferWall(Actor* actor, const float* normalXyz, float dist) {
    if ((actor == NULL) || !On()) {
        return;
    }
    sWall = { actor, { normalXyz[0], normalXyz[1], normalXyz[2] }, dist };
}

extern "C" Actor* VrCombat_CarryTakeGrab(void) {
    if (!sGrabReady || (sTarget == NULL) || (sTargetKind == VrCarry::Kind::Block)) {
        return NULL;
    }
    sGrabReady = false;
    return sTarget;
}

extern "C" bool VrCombat_CarryTakeWallGrab(Actor* actor) {
    if (!sGrabReady || (actor == NULL) || (sTarget != actor) || (sTargetKind != VrCarry::Kind::Block)) {
        return false;
    }
    sGrabReady = false;
    sWallGrips = true;
    sPushDir = 0;
    return true;
}

extern "C" bool VrCombat_CarryWallHeld(void) {
    return VR_IsInitialized() && sWallGrips && (sTargetKind == VrCarry::Kind::Block);
}

extern "C" int32_t VrCombat_CarryPushDir(void) {
    return VrCombat_CarryWallHeld() ? sPushDir : 0;
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
    return HeldPosNow(outXyz);
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
    // Both hands at objects: the grips lift, they do not draw the sword.
    return VR_IsInitialized() && ((sHold.Hand() >= 0) || sWallGrips || (sInReach[0] && sInReach[1]));
}

extern "C" bool VrCombat_CarryGripConsumed(int32_t vrHand, uint16_t vrBtnMask) {
    if (!(vrBtnMask & VR_BTN_GRIP) || !VR_IsInitialized() || (vrHand < 0) || (vrHand > 1)) {
        return false;
    }
    return (sHold.Hand() == vrHand) || sHold.TwoHands() || sWallGrips || sInReach[vrHand];
}
