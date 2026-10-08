#pragma once

// Pure rule behind the physical Megaton Hammer ground hit (VrSwing.cpp): no game types, so the
// host tests in port/soh/tests/vr/VrHammerSlamTests.cpp can build it alone.
//
// The original game starts the ground hit (screen shake, shock wave, enemy stun) at a fixed frame
// of the hammer attack animation, and only when the floor is within 40 units of Link's feet
// (Player_Action_808502D0). In VR there is no animation: a slam is the hammer head arriving on a
// floor surface fast enough, moving down. One slam per contact: the head must leave the surface
// before it can slam again.

namespace VrHammerSlam {

struct Params {
    float minDownSpeedMps = 4.0f;  // downward speed of the head, physical m/s
    float floorMinNormalY = 0.5f;  // steeper surfaces are walls, not floor
    float maxFeetOffset = 40.0f;   // the original game's floor window around Link's feet, game units
};

// One game tick of the hammer head.
struct Sample {
    float headDownSpeedMps = 0.0f; // max downward speed this tick (negative = moving up)
    bool headOnSurface = false;    // the probe under the head found level geometry
    float surfaceNormalY = 0.0f;   // of the surface the probe found
    float surfaceYRelFeet = 0.0f;  // surface height minus Link's feet height, game units
};

class Detector {
  public:
    // True on the tick the slam happens.
    bool Update(const Params& p, const Sample& s) {
        if (!s.headOnSurface) {
            mLatched = false;
            return false;
        }
        if (mLatched) {
            return false;
        }
        const bool floor = s.surfaceNormalY >= p.floorMinNormalY;
        const bool nearFeet = s.surfaceYRelFeet > -p.maxFeetOffset && s.surfaceYRelFeet < p.maxFeetOffset;
        if (floor && nearFeet && s.headDownSpeedMps >= p.minDownSpeedMps) {
            mLatched = true;
            return true;
        }
        return false;
    }

    void Reset() {
        mLatched = false;
    }

  private:
    bool mLatched = false;
};

} // namespace VrHammerSlam
