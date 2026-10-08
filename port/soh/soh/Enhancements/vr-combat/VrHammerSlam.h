#pragma once

// Rule for the ground hit of the physical Megaton Hammer. No game types: the host tests in
// port/soh/tests/vr/VrHammerSlamTests.cpp build it alone.
//
// A ground hit occurs when the hammer head moves down fast and touches a floor. The floor must be
// within 40 units of the feet of Link, as in the original game (Player_Action_808502D0). The head
// must leave the surface before the next ground hit.

namespace VrHammerSlam {

struct Params {
    float minDownSpeedMps = 4.0f;  // m/s
    float floorMinNormalY = 0.5f;  // a steeper surface is a wall
    float maxFeetOffset = 40.0f;   // game units
};

// One game tick of the hammer head.
struct Sample {
    float headDownSpeedMps = 0.0f; // maximum in this tick; negative when the head moves up
    bool headOnSurface = false;    // the line test under the head found a surface
    float surfaceNormalY = 0.0f;
    float surfaceYRelFeet = 0.0f;  // surface height minus the height of the feet, game units
};

class Detector {
  public:
    // True on the tick of the ground hit.
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
