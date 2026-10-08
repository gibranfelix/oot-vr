#pragma once

// Bow aim math (VrBowAim.cpp). No game types. Tests: port/soh/tests/vr/VrBowAimTests.cpp.

#include <cmath>

namespace VrBowAim {

struct Vec3 {
    float x, y, z;
};

// The nock in bow hand space, from gLinkAdultBowStringDL and sBowStringData. A full draw moves
// the nock to kNockFullDrawY. The arrow points along hand +Y.
constexpr Vec3 kNockAtRest = { 360.0f, -360.4f, 0.0f };
constexpr float kNockFullDrawY = -1160.0f;

// handMf: the bow hand matrix, game MtxF layout (mf[3] is the translation). It can contain a scale
// and a mirror. draw: Player.unk_858, 0 to 1.
inline void ArrowOnBow(const float handMf[4][4], float draw, Vec3* pos, Vec3* dir) {
    draw = (draw < 0.0f) ? 0.0f : ((draw > 1.0f) ? 1.0f : draw);
    const float nockY = kNockAtRest.y + kNockFullDrawY * draw;
    float out[3];
    for (int i = 0; i < 3; i++) {
        out[i] = kNockAtRest.x * handMf[0][i] + nockY * handMf[1][i] + kNockAtRest.z * handMf[2][i] + handMf[3][i];
    }
    const float* y = handMf[1];
    const float len = std::sqrt(y[0] * y[0] + y[1] * y[1] + y[2] * y[2]);
    const float inv = (len > 0.0f) ? (1.0f / len) : 0.0f;
    *pos = { out[0], out[1], out[2] };
    *dir = { y[0] * inv, y[1] * inv, y[2] * inv };
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
