extern "C" {
#include "z64.h"
#include "macros.h"
#include "functions.h"
#include "variables.h"
}

#include "VrCombat.h"
#include "VrBowAim.h"

#include <libultraship/bridge/consolevariablebridge.h>
#include <vr_interface.h>

static void CopyOut(const VrBowAim::Vec3& pos, const VrBowAim::Vec3& dir, float* outPosXyz, float* outDirXyz) {
    outPosXyz[0] = pos.x;
    outPosXyz[1] = pos.y;
    outPosXyz[2] = pos.z;
    outDirXyz[0] = dir.x;
    outDirXyz[1] = dir.y;
    outDirXyz[2] = dir.z;
}

extern "C" void VrCombat_ArrowOnBow(const float* handMf16, float draw, float* outPosXyz, float* outDirXyz) {
    const float(*mf)[4] = reinterpret_cast<const float(*)[4]>(handMf16);
    VrBowAim::Vec3 pos;
    VrBowAim::Vec3 dir;
    VrBowAim::ArrowOnBow(mf, draw, &pos, &dir);
    CopyOut(pos, dir, outPosXyz, outDirXyz);
}

extern "C" void VrCombat_SeedOnSlingshot(const float* handMf16, float draw, bool childTilt, float* outPosXyz,
                                         float* outDirXyz) {
    const float(*mf)[4] = reinterpret_cast<const float(*)[4]>(handMf16);
    VrBowAim::Vec3 pos;
    VrBowAim::Vec3 dir;
    VrBowAim::SeedOnSlingshot(mf, draw, childTilt, &pos, &dir);
    CopyOut(pos, dir, outPosXyz, outDirXyz);
}

// Same rule as Player_VrMotionAimOn in z_player_lib.c.
bool VrCombat::WeaponAimOn() {
    return VR_IsInitialized() && VR_GetFirstPerson() && CVarGetInteger("gVrMotionHands", 1) &&
           CVarGetInteger("gVrWeaponAim", 1);
}

extern "C" int32_t VrCombat_ArrowExtraFrames(void) {
    return VrCombat::WeaponAimOn() ? VrBowAim::kVrExtraFrames : 0;
}

extern "C" bool VrCombat_PredictShotHit(PlayState* play, const float* posXyz, const float* dirXyz, bool seed,
                                        float* outHitXyz) {
    // Same line test as EnArrow_Fly.
    auto lineTest = [play](const VrBowAim::Vec3& a, const VrBowAim::Vec3& b, VrBowAim::Vec3* hit) {
        Vec3f from = { a.x, a.y, a.z };
        Vec3f to = { b.x, b.y, b.z };
        Vec3f point;
        CollisionPoly* poly;
        s32 bgId;
        if (!BgCheck_ProjectileLineTest(&play->colCtx, &from, &to, &point, &poly, true, true, true, true, &bgId)) {
            return false;
        }
        *hit = { point.x, point.y, point.z };
        return true;
    };
    const VrBowAim::Flight& flight = seed ? VrBowAim::kSeedFlight
                                          : ((VrCombat_ArrowExtraFrames() > 0) ? VrBowAim::kArrowFlightVr
                                                                               : VrBowAim::kArrowFlight);
    VrBowAim::Vec3 hit;
    if (!VrBowAim::PredictHit({ posXyz[0], posXyz[1], posXyz[2] }, { dirXyz[0], dirXyz[1], dirXyz[2] },
                              R_UPDATE_RATE * 0.5f, flight, lineTest, &hit)) {
        return false;
    }
    outHitXyz[0] = hit.x;
    outHitXyz[1] = hit.y;
    outHitXyz[2] = hit.z;
    return true;
}
