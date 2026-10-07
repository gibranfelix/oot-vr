#pragma once

// SOH [VR] Automatic refresh rate (issue #83): 90 Hz, or 72 Hz when frames are late. Call Update one
// time each second. No OpenXR, so the host tests can use it.
class RefreshRateGovernor {
  public:
    static constexpr int kHighHz = 90;
    static constexpr int kLowHz = 72;

    struct Second {
        float frameHz;       // frames that the game submitted in this second
        float averageWorkMs; // average work of these frames, without the wait in xrWaitFrame
    };

    // Returns the rate to request.
    int Update(const Second& s) {
        mSecondsSinceChange++;
        if (mTarget == kHighHz) {
            // Late: fewer than 90% of the frames arrived.
            mLateSeconds = s.frameHz < kLateRatio * kHighHz ? mLateSeconds + 1 : 0;
            if (mLateSeconds >= kSecondsToDrop && mSecondsSinceChange >= kMinSecondsBetweenChanges) {
                Change(kLowHz);
            }
        } else {
            // Smooth: on time at 72 Hz, and the average work fits in 80% of a 90 Hz frame. With
            // Stereo Render Divisor 2, one frame can be long while the pair of frames still fits.
            const bool smooth =
                s.frameHz >= kOnTimeRatio * kLowHz && s.averageWorkMs <= kRiseWorkRatio * 1000.0f / kHighHz;
            mSmoothSeconds = smooth ? mSmoothSeconds + 1 : 0;
            if (mSmoothSeconds >= kSecondsToRise && mSecondsSinceChange >= kMinSecondsBetweenChanges) {
                Change(kHighHz);
            }
        }
        return mTarget;
    }

    int Target() const {
        return mTarget;
    }

    // Start again at the high rate, for example when the player selects "Automatic".
    void Reset() {
        Change(kHighHz);
    }

  private:
    static constexpr float kLateRatio = 0.9f;
    static constexpr float kOnTimeRatio = 0.95f;
    static constexpr float kRiseWorkRatio = 0.8f;
    static constexpr int kSecondsToDrop = 2;
    static constexpr int kSecondsToRise = 10;
    static constexpr int kMinSecondsBetweenChanges = 10;

    void Change(int hz) {
        mTarget = hz;
        mSecondsSinceChange = 0;
        mLateSeconds = 0;
        mSmoothSeconds = 0;
    }

    int mTarget = kHighHz;
    int mSecondsSinceChange = 0;
    int mLateSeconds = 0;
    int mSmoothSeconds = 0;
};
