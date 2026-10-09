// Host unit tests for VrBowAim.h (pure math, no game or OpenXR). The game build does not compile
// this file. Run it from the repository root:
//
//   g++ -std=c++17 -I port/soh/soh/Enhancements/vr-combat port/soh/tests/vr/VrBowAimTests.cpp -o /tmp/vr_bow_aim_tests
//   /tmp/vr_bow_aim_tests

#include "VrBowAim.h"

#include <cmath>
#include <cstdio>
#include <vector>

using namespace VrBowAim;

static int sFailures = 0;

#define EXPECT(cond)                                                    \
    do {                                                                \
        if (!(cond)) {                                                  \
            std::printf("%s:%d: FAILED: %s\n", __FILE__, __LINE__, #cond); \
            sFailures++;                                                \
        }                                                               \
    } while (0)

static bool Near(float a, float b, float eps = 0.01f) {
    return std::fabs(a - b) <= eps;
}

static bool NearVec(const Vec3& a, const Vec3& b, float eps = 0.01f) {
    return Near(a.x, b.x, eps) && Near(a.y, b.y, eps) && Near(a.z, b.z, eps);
}

// Game MtxF layout: a point p goes to p.x * mf[0] + p.y * mf[1] + p.z * mf[2] + mf[3].
static void Identity(float mf[4][4]) {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            mf[r][c] = (r == c) ? 1.0f : 0.0f;
        }
    }
}

// A bow hand with Link's model scale (0.01), at (100, 50, -20). The hand Y axis points along
// world -Z (the forward direction of the game camera at yaw 0x8000).
static void BowHand(float mf[4][4], bool mirrorZ) {
    Identity(mf);
    const float s = 0.01f;
    mf[0][0] = s;                       // hand X -> world X
    mf[1][0] = 0.0f, mf[1][1] = 0.0f;   // hand Y -> world -Z
    mf[1][2] = -s;
    mf[2][1] = mirrorZ ? -s : s;        // hand Z -> world Y (reflected for a mirrored hand)
    mf[2][2] = 0.0f;
    mf[3][0] = 100.0f;
    mf[3][1] = 50.0f;
    mf[3][2] = -20.0f;
}

static void TestArrowLiesAlongTheBowHandY() {
    float mf[4][4];
    BowHand(mf, false);
    Vec3 pos;
    Vec3 dir;
    ArrowOnBow(mf, 0.0f, &pos, &dir);

    EXPECT(NearVec(dir, { 0.0f, 0.0f, -1.0f }));
    // Nock at rest: hand (360, -360.4, 0) -> world (100 + 3.6, 50, -20 + 3.604).
    EXPECT(NearVec(pos, { 103.6f, 50.0f, -16.396f }));
}

static void TestDrawPullsTheNockBack() {
    float mf[4][4];
    BowHand(mf, false);
    Vec3 rest;
    Vec3 full;
    Vec3 dir;
    ArrowOnBow(mf, 0.0f, &rest, &dir);
    ArrowOnBow(mf, 1.0f, &full, &dir);

    // A full draw moves the nock 1160 hand units (11.6 world units) back, against the arrow.
    EXPECT(Near(full.z - rest.z, 11.6f));
    EXPECT(Near(full.x, rest.x));
    EXPECT(Near(full.y, rest.y));
    EXPECT(NearVec(dir, { 0.0f, 0.0f, -1.0f }));
}

static void TestDrawIsClamped() {
    float mf[4][4];
    BowHand(mf, false);
    Vec3 a;
    Vec3 b;
    Vec3 dir;
    ArrowOnBow(mf, 1.0f, &a, &dir);
    ArrowOnBow(mf, 3.0f, &b, &dir);
    EXPECT(NearVec(a, b));
    ArrowOnBow(mf, 0.0f, &a, &dir);
    ArrowOnBow(mf, -2.0f, &b, &dir);
    EXPECT(NearVec(a, b));
}

static void TestMirroredHandKeepsTheDirection() {
    float plain[4][4];
    float mirrored[4][4];
    BowHand(plain, false);
    BowHand(mirrored, true);
    Vec3 p0;
    Vec3 d0;
    Vec3 p1;
    Vec3 d1;
    ArrowOnBow(plain, 0.5f, &p0, &d0);
    ArrowOnBow(mirrored, 0.5f, &p1, &d1);
    EXPECT(NearVec(p0, p1));
    EXPECT(NearVec(d0, d1));
}

static void TestDirectionIsUnitLength() {
    float mf[4][4];
    BowHand(mf, false);
    // Tilt the hand: hand Y now points forward and up at 45 degrees.
    mf[1][1] = 0.01f;
    Vec3 pos;
    Vec3 dir;
    ArrowOnBow(mf, 0.0f, &pos, &dir);
    const float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    EXPECT(Near(len, 1.0f, 0.0001f));
    EXPECT(Near(dir.y, std::sqrt(0.5f), 0.0001f));
    EXPECT(Near(dir.z, -std::sqrt(0.5f), 0.0001f));
}

// Records each segment and reports a hit when a segment crosses the plane z = wallZ.
struct WallZ {
    float wallZ;
    std::vector<Vec3>* starts;
    bool operator()(const Vec3& a, const Vec3& b, Vec3* hit) const {
        if (starts != nullptr) {
            starts->push_back(a);
        }
        if ((a.z - wallZ) * (b.z - wallZ) > 0.0f || a.z == b.z) {
            return false;
        }
        const float t = (wallZ - a.z) / (b.z - a.z);
        *hit = { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, wallZ };
        return true;
    }
};

static void TestHitsAWallStraightAhead() {
    Vec3 hit;
    // EnArrow speed 150, R_UPDATE_RATE 3: 225 units in each frame. The wall is in the first frame.
    const bool found = PredictHit({ 0, 0, 0 }, { 0, 0, -1 }, 1.5f, kArrowFlight, WallZ{ -200.0f, nullptr }, &hit);
    EXPECT(found);
    EXPECT(NearVec(hit, { 0.0f, 0.0f, -200.0f }));
}

static void TestNoHitOutOfRange() {
    Vec3 hit = { 7, 7, 7 };
    std::vector<Vec3> starts;
    const bool found =
        PredictHit({ 0, 0, 0 }, { 0, 0, -1 }, 1.5f, kArrowFlight, WallZ{ -100000.0f, &starts }, &hit);
    EXPECT(!found);
    // EnArrow_Fly moves 11 times before its timer kills the arrow.
    EXPECT(starts.size() == 11);
    EXPECT(NearVec(hit, { 7, 7, 7 }));
}

static void TestGravityStartsOnTheFifthMove() {
    std::vector<Vec3> starts;
    Vec3 hit;
    PredictHit({ 0, 0, 0 }, { 0, 0, -1 }, 1.0f, kArrowFlight, WallZ{ -100000.0f, &starts }, &hit);
    EXPECT(starts.size() == 11);
    // Moves 1 to 4 are straight. Move 5 is the first with gravity (timer 7 < 7.2).
    EXPECT(Near(starts[4].y, 0.0f));
    EXPECT(Near(starts[4].z, -600.0f));
    EXPECT(Near(starts[5].y, -0.4f));
    // Total drop after 11 moves at step scale 1: 0.4 * (1 + 2 + ... + 6) before the last move.
    EXPECT(Near(starts[10].y, -0.4f * 21.0f));
}

static void TestVrFlightIsLongerWithTheSameArc() {
    std::vector<Vec3> vanilla;
    std::vector<Vec3> vr;
    Vec3 hit;
    PredictHit({ 0, 0, 0 }, { 0, 0, -1 }, 1.0f, kArrowFlight, WallZ{ -100000.0f, &vanilla }, &hit);
    PredictHit({ 0, 0, 0 }, { 0, 0, -1 }, 1.0f, kArrowFlightVr, WallZ{ -100000.0f, &vr }, &hit);
    EXPECT(vr.size() == vanilla.size() + kVrExtraFrames);
    // The VR flight follows the vanilla arc, and continues after it.
    for (size_t i = 0; i < vanilla.size(); i++) {
        EXPECT(NearVec(vr[i], vanilla[i]));
    }
}

static void TestDropFollowsTheVanillaFlight() {
    // The aim mark is on a far wall below the line of the bow, because the arrow drops.
    Vec3 hit;
    const bool found =
        PredictHit({ 0, 0, 0 }, { 0, 0, -1 }, 1.5f, kArrowFlight, WallZ{ -2000.0f, nullptr }, &hit);
    EXPECT(found);
    EXPECT(hit.y < 0.0f);
    EXPECT(hit.y > -20.0f);
}

static void TestUpwardShotKeepsItsPitch() {
    Vec3 hit;
    const float k = std::sqrt(0.5f);
    const bool found = PredictHit({ 0, 0, 0 }, { 0, k, -k }, 1.5f, kArrowFlight, WallZ{ -300.0f, nullptr }, &hit);
    EXPECT(found);
    EXPECT(Near(hit.y, 300.0f, 0.5f));
}

static void TestSeedLiesAlongTheSlingshotHandY() {
    float mf[4][4];
    BowHand(mf, false);
    Vec3 pos;
    Vec3 dir;
    SeedOnSlingshot(mf, 0.0f, true, &pos, &dir);

    EXPECT(NearVec(dir, { 0.0f, 0.0f, -1.0f }));
    // Pouch at rest: string (-9.5, 0, 55) -> hand (596.5, 236, 55).
    EXPECT(NearVec(pos, { 105.965f, 50.55f, -22.36f }));
}

static void TestDrawPullsThePouchBack() {
    float mf[4][4];
    BowHand(mf, false);
    Vec3 full;
    Vec3 dir;
    SeedOnSlingshot(mf, 1.0f, false, &full, &dir);

    // Full draw, no tilt: string (-9.5, -1324.5, 55) -> hand (596.5, -1088.5, 55).
    EXPECT(NearVec(full, { 105.965f, 50.55f, -9.115f }));
    EXPECT(NearVec(dir, { 0.0f, 0.0f, -1.0f }));
}

static void TestChildStringTiltMovesThePouchNotTheDirection() {
    float mf[4][4];
    BowHand(mf, false);
    Vec3 straight;
    Vec3 tilted;
    Vec3 d0;
    Vec3 d1;
    SeedOnSlingshot(mf, 1.0f, false, &straight, &d0);
    SeedOnSlingshot(mf, 1.0f, true, &tilted, &d1);

    // The tilt moves the pouch toward hand -X. The direction does not change.
    EXPECT(tilted.x < straight.x - 2.0f);
    EXPECT(NearVec(d0, d1));
}

static void TestSeedFlightIsTheVanillaSeedFlight() {
    std::vector<Vec3> starts;
    Vec3 hit;
    PredictHit({ 0, 0, 0 }, { 0, 0, -1 }, 1.0f, kSeedFlight, WallZ{ -100000.0f, &starts }, &hit);
    // EnArrow_Shoot: speed 80, timer 15, thus 14 moves.
    EXPECT(starts.size() == 14);
    EXPECT(Near(starts[1].z, -80.0f));
    // Gravity starts when the timer is less than 7.2: the move with timer 7.
    EXPECT(Near(starts[7].y, 0.0f));
    EXPECT(Near(starts[8].y, -0.4f));
}

int main() {
    TestArrowLiesAlongTheBowHandY();
    TestDrawPullsTheNockBack();
    TestDrawIsClamped();
    TestMirroredHandKeepsTheDirection();
    TestDirectionIsUnitLength();
    TestHitsAWallStraightAhead();
    TestNoHitOutOfRange();
    TestGravityStartsOnTheFifthMove();
    TestVrFlightIsLongerWithTheSameArc();
    TestDropFollowsTheVanillaFlight();
    TestUpwardShotKeepsItsPitch();
    TestSeedLiesAlongTheSlingshotHandY();
    TestDrawPullsThePouchBack();
    TestChildStringTiltMovesThePouchNotTheDirection();
    TestSeedFlightIsTheVanillaSeedFlight();

    if (sFailures == 0) {
        std::printf("VrBowAim: all tests passed\n");
        return 0;
    }
    std::printf("VrBowAim: %d failures\n", sFailures);
    return 1;
}
