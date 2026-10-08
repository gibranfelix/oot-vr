extern "C" {
#include "z64.h"
#include "macros.h"
#include "functions.h"
#include "variables.h"
}

#include "VrCombat.h"
#include "VrBowAim.h"

extern "C" void VrCombat_ArrowOnBow(const float* handMf16, float draw, float* outPosXyz, float* outDirXyz) {
    const float(*mf)[4] = reinterpret_cast<const float(*)[4]>(handMf16);
    VrBowAim::Vec3 pos;
    VrBowAim::Vec3 dir;
    VrBowAim::ArrowOnBow(mf, draw, &pos, &dir);
    outPosXyz[0] = pos.x;
    outPosXyz[1] = pos.y;
    outPosXyz[2] = pos.z;
    outDirXyz[0] = dir.x;
    outDirXyz[1] = dir.y;
    outDirXyz[2] = dir.z;
}

extern "C" bool VrCombat_PredictArrowHit(PlayState* play, const float* posXyz, const float* dirXyz,
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
    VrBowAim::Vec3 hit;
    if (!VrBowAim::PredictHit({ posXyz[0], posXyz[1], posXyz[2] }, { dirXyz[0], dirXyz[1], dirXyz[2] },
                              R_UPDATE_RATE * 0.5f, VrBowAim::kArrowFlight, lineTest, &hit)) {
        return false;
    }
    outHitXyz[0] = hit.x;
    outHitXyz[1] = hit.y;
    outHitXyz[2] = hit.z;
    return true;
}
