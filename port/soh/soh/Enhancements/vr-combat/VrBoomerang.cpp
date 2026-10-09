extern "C" {
#include "z64.h"
#include "macros.h"
#include "functions.h"
#include "variables.h"
extern PlayState* gPlayState;
}

#include "VrCombat.h"
#include "VrBowAim.h"
#include "VrBoomerangThrow.h"

#include <libultraship/bridge/consolevariablebridge.h>
#include <vr_interface.h>

namespace {

VrBoomerangThrow::Detector sDetector;
// Ready for one tick: the next Player_Update takes it.
bool sGripThrowReady = false;
VrBoomerangThrow::Throw sGripThrow;

// The boomerang is in the sword hand (HeldItemVrHand in VrItemSelect.cpp).
int BoomerangHand() {
    return CVarGetInteger("gVrLeftHanded", 0) ? VR_HAND_LEFT : VR_HAND_RIGHT;
}

bool BoomerangHeld(Player* player) {
    return (player != NULL) && (player->heldItemAction == PLAYER_IA_BOOMERANG);
}

int32_t MovesForRange(float range) {
    return VrBoomerangThrow::MovesForRange(range, R_UPDATE_RATE * 0.5f);
}

bool AimRay(VrBowAim::Vec3* pos, VrBowAim::Vec3* dir) {
    float p[3];
    float d[3];
    if (!VrCombat::WeaponAimOn() || !VR_GetAimRay(BoomerangHand(), p, d)) {
        return false;
    }
    *pos = { p[0], p[1], p[2] };
    *dir = { d[0], d[1], d[2] };
    return true;
}

} // namespace

void VrCombat::Boomerang_OnPlayerUpdate(Player* player) {
    sGripThrowReady = false;
    if (!VR_IsInitialized()) {
        return;
    }
    const int hand = BoomerangHand();
    const TickPath& path = GetTickPath(hand);
    if (path.count == 0) {
        sDetector.Clear();
    }
    for (int i = 0; i < path.count; i++) {
        const VrHandSample& s = path.samples[i];
        sDetector.AddSample(
            { { s.pos[0], s.pos[1], s.pos[2] }, { s.linVelMps[0], s.linVelMps[1], s.linVelMps[2] }, s.timeNs });
    }
    VrBoomerangThrow::Input in;
    in.canThrow = WeaponAimOn() && VrCombat_InPlay() && BoomerangHeld(player) &&
                  !(player->stateFlags1 & PLAYER_STATE1_BOOMERANG_THROWN);
    in.gripHeld = (VR_GetControllerButton(hand) & VR_BTN_GRIP) != 0;
    in.otherGripHeld = (VR_GetControllerButton(hand ^ 1) & VR_BTN_GRIP) != 0;
    const float yaw = (player != NULL) ? (player->actor.shape.rot.y * (M_PI / 0x8000)) : 0.0f;
    in.forward = { sinf(yaw), 0.0f, cosf(yaw) };
    sGripThrowReady = sDetector.Update(in, &sGripThrow);
}

static void MakeThrow(const VrBoomerangThrow::Vec3& pos, const VrBoomerangThrow::Vec3& dir, float range,
                      const float* targetXyz, VrCombatBoomerangThrow* out) {
    out->pos[0] = pos.x;
    out->pos[1] = pos.y;
    out->pos[2] = pos.z;
    out->at[0] = pos.x + dir.x * 100.0f;
    out->at[1] = pos.y + dir.y * 100.0f;
    out->at[2] = pos.z + dir.z * 100.0f;
    out->moves = MovesForRange(range);
    out->toTarget = (targetXyz != nullptr) &&
                    VrBoomerangThrow::TowardTarget(pos, dir, { targetXyz[0], targetXyz[1], targetXyz[2] });
}

extern "C" bool VrCombat_BoomerangTriggerThrow(const float* targetXyz, VrCombatBoomerangThrow* out) {
    VrBowAim::Vec3 pos;
    VrBowAim::Vec3 dir;
    if (!AimRay(&pos, &dir)) {
        return false;
    }
    MakeThrow({ pos.x, pos.y, pos.z }, { dir.x, dir.y, dir.z }, VR_BOOMERANG_TRIGGER_RANGE, targetXyz, out);
    return true;
}

extern "C" bool VrCombat_BoomerangTakeGripThrow(const float* targetXyz, VrCombatBoomerangThrow* out) {
    if (!sGripThrowReady) {
        return false;
    }
    sGripThrowReady = false;
    MakeThrow(sGripThrow.pos, sGripThrow.dir, sGripThrow.range, targetXyz, out);
    return true;
}

extern "C" bool VrCombat_BoomerangAimMark(PlayState* play, float* outHitXyz) {
    VrBowAim::Vec3 pos;
    VrBowAim::Vec3 dir;
    if (!AimRay(&pos, &dir)) {
        return false;
    }
    // Same line test as EnBoom_Fly.
    auto lineTest = [play](const VrBowAim::Vec3& a, const VrBowAim::Vec3& b, VrBowAim::Vec3* hit) {
        Vec3f from = { a.x, a.y, a.z };
        Vec3f to = { b.x, b.y, b.z };
        Vec3f point;
        CollisionPoly* poly;
        s32 bgId;
        if (!BgCheck_EntityLineTest1(&play->colCtx, &from, &to, &point, &poly, true, true, true, true, &bgId)) {
            return false;
        }
        *hit = { point.x, point.y, point.z };
        return true;
    };
    VrBowAim::Vec3 hit;
    if (!VrBowAim::PredictHit(pos, dir, R_UPDATE_RATE * 0.5f, VrBowAim::kBoomerangFlightVr, lineTest, &hit)) {
        return false;
    }
    outHitXyz[0] = hit.x;
    outHitXyz[1] = hit.y;
    outHitXyz[2] = hit.z;
    return true;
}

extern "C" bool VrCombat_BoomerangGripConsumed(int32_t vrHand, uint16_t vrBtnMask) {
    if (!(vrBtnMask & VR_BTN_GRIP) || (vrHand != BoomerangHand()) || !VrCombat::WeaponAimOn() || (gPlayState == NULL)) {
        return false;
    }
    Player* player = GET_PLAYER(gPlayState);
    return BoomerangHeld(player) && !(player->stateFlags1 & PLAYER_STATE1_BOOMERANG_THROWN);
}
