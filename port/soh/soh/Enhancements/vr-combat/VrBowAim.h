#pragma once

// Bow, slingshot, and boomerang aim math (VrBowAim.cpp). No game types. Tests: port/soh/tests/vr/VrBowAimTests.cpp.

#include "VrBoomerangFlight.h"

#include <cmath>

namespace VrBowAim {

struct Vec3 {
    float x, y, z;
};

// The nock in bow hand space, from gLinkAdultBowStringDL and sBowStringData. A full draw moves
// the nock to kNockFullDrawY. The arrow points along hand +Y.
constexpr Vec3 kNockAtRest = { 360.0f, -360.4f, 0.0f };
constexpr float kNockFullDrawY = -1160.0f;

// The slingshot string origin (sBowStringData), and the pouch at a full draw in string space
// (gLinkChildSlingshotStringDL). The slingshot model is flat in the plane hand Y = 236. Thus, the
// seed points along hand +Y.
constexpr Vec3 kSlingshotString = { 606.0f, 236.0f, 0.0f };
constexpr Vec3 kPouchFullDraw = { -9.5f, -1324.5f, 55.0f };
// Child Link: Matrix_RotateZ(draw * kChildStringTilt) turns the string before the draw scale.
constexpr float kChildStringTilt = -0.2f;

inline float ClampDraw(float draw) {
    return (draw < 0.0f) ? 0.0f : ((draw > 1.0f) ? 1.0f : draw);
}

// Returns the world position of p (hand space) and the unit hand +Y axis.
inline void HandPointAlongY(const float handMf[4][4], const Vec3& p, Vec3* pos, Vec3* dir) {
    float out[3];
    for (int i = 0; i < 3; i++) {
        out[i] = p.x * handMf[0][i] + p.y * handMf[1][i] + p.z * handMf[2][i] + handMf[3][i];
    }
    const float* y = handMf[1];
    const float len = std::sqrt(y[0] * y[0] + y[1] * y[1] + y[2] * y[2]);
    const float inv = (len > 0.0f) ? (1.0f / len) : 0.0f;
    *pos = { out[0], out[1], out[2] };
    *dir = { y[0] * inv, y[1] * inv, y[2] * inv };
}

// handMf: the bow hand matrix, game MtxF layout (mf[3] is the translation). It can contain a scale
// and a mirror. draw: Player.unk_858, 0 to 1.
inline void ArrowOnBow(const float handMf[4][4], float draw, Vec3* pos, Vec3* dir) {
    draw = ClampDraw(draw);
    HandPointAlongY(handMf, { kNockAtRest.x, kNockAtRest.y + kNockFullDrawY * draw, kNockAtRest.z }, pos, dir);
}

// The seed in the pouch, as Player_PostLimbDrawGameplay draws the string. childTilt: !LINK_IS_ADULT.
inline void SeedOnSlingshot(const float handMf[4][4], float draw, bool childTilt, Vec3* pos, Vec3* dir) {
    draw = ClampDraw(draw);
    const float angle = childTilt ? (kChildStringTilt * draw) : 0.0f;
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    const Vec3 p = { kSlingshotString.x + kPouchFullDraw.x * c - kPouchFullDraw.y * s,
                     kSlingshotString.y + (kPouchFullDraw.x * s + kPouchFullDraw.y * c) * draw,
                     kSlingshotString.z + kPouchFullDraw.z };
    HandPointAlongY(handMf, p, pos, dir);
}

// The flight of EnArrow_Fly. Gravity starts when the timer is less than gravityBelowTimer.
struct Flight {
    float speed;
    int timer;
    float gravityBelowTimer;
    float gravity;
    float minVelocityY;
};

// All arrow types: normal, fire, ice, and light.
constexpr Flight kArrowFlight = { 150.0f, 12, 7.2f, -0.4f, -150.0f };

// EnArrow_Shoot for ARROW_SEED.
constexpr Flight kSeedFlight = { 80.0f, 15, 7.2f, -0.4f, -150.0f };

// The boomerang trigger throw in VR: 10 moves of 36 units (VR_BOOMERANG_TRIGGER_RANGE), no
// gravity. PredictHit moves timer - 1 times.
constexpr Flight kBoomerangFlightVr = { VR_BOOMERANG_SPEED, 11, 0.0f, 0.0f, -150.0f };

// In VR, the arrow flies kVrExtraFrames more, thus it hits what the player sees. The arc is the
// same: gravity starts on the same move.
constexpr int kVrExtraFrames = 24;
constexpr Flight kArrowFlightVr = { kArrowFlight.speed, kArrowFlight.timer + kVrExtraFrames,
                                    kArrowFlight.gravityBelowTimer + kVrExtraFrames, kArrowFlight.gravity,
                                    kArrowFlight.minVelocityY };

// Returns the first hit of the flight from pos along the unit vector dir. stepScale:
// R_UPDATE_RATE * 0.5, as in Actor_UpdatePos. lineTest(a, b, &hit) tests one segment.
template <typename LineTest>
bool PredictHit(Vec3 pos, Vec3 dir, float stepScale, const Flight& flight, LineTest lineTest, Vec3* hit) {
    const float velX = dir.x * flight.speed;
    const float velZ = dir.z * flight.speed;
    float velY = dir.y * flight.speed;
    float gravity = 0.0f;
    for (int timer = flight.timer - 1; timer > 0; timer--) {
        if (timer < flight.gravityBelowTimer) {
            gravity = flight.gravity;
        }
        velY += gravity;
        if (velY < flight.minVelocityY) {
            velY = flight.minVelocityY;
        }
        const Vec3 next = { pos.x + velX * stepScale, pos.y + velY * stepScale, pos.z + velZ * stepScale };
        Vec3 point;
        if (lineTest(pos, next, &point)) {
            *hit = point;
            return true;
        }
        pos = next;
    }
    return false;
}

} // namespace VrBowAim
