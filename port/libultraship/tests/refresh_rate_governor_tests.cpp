#include <gtest/gtest.h>
#include "fast/refresh_rate_governor.h"

// SOH [VR] Automatic refresh rate between 90 Hz and 72 Hz (issue #83).

namespace {

// One second at full rate, with 5 ms of work in each frame (the 90 Hz budget is 11.1 ms).
RefreshRateGovernor::Second Smooth(int hz) {
    return { (float)hz, 5.0f };
}

// One second with late frames.
RefreshRateGovernor::Second Late(float frameHz) {
    return { frameHz, 14.0f };
}

void Feed(RefreshRateGovernor& g, const RefreshRateGovernor::Second& s, int seconds) {
    for (int i = 0; i < seconds; i++) {
        g.Update(s);
    }
}

} // namespace

TEST(RefreshRateGovernorTest, StartsAtTheHighRate) {
    RefreshRateGovernor g;
    EXPECT_EQ(g.Target(), 90);
}

TEST(RefreshRateGovernorTest, StaysHighWhileFramesAreOnTime) {
    RefreshRateGovernor g;
    Feed(g, Smooth(90), 30);
    EXPECT_EQ(g.Target(), 90);
}

TEST(RefreshRateGovernorTest, OneLateSecondDoesNotDrop) {
    RefreshRateGovernor g;
    Feed(g, Smooth(90), 15);
    g.Update(Late(60));
    g.Update(Smooth(90));
    EXPECT_EQ(g.Target(), 90);
}

TEST(RefreshRateGovernorTest, TwoLateSecondsDropToTheLowRate) {
    RefreshRateGovernor g;
    Feed(g, Smooth(90), 15);
    g.Update(Late(60));
    EXPECT_EQ(g.Update(Late(60)), 72);
}

TEST(RefreshRateGovernorTest, NinetyPercentOfTheRateIsNotLate) {
    RefreshRateGovernor g;
    Feed(g, Smooth(90), 15);
    Feed(g, { 81.0f, 9.0f }, 5);
    EXPECT_EQ(g.Target(), 90);
}

TEST(RefreshRateGovernorTest, DoesNotChangeTwoTimesInTenSeconds) {
    RefreshRateGovernor g;
    Feed(g, Smooth(90), 15);
    Feed(g, Late(60), 2);
    ASSERT_EQ(g.Target(), 72);
    // At once smooth again: the rise must wait for the minimum time between changes.
    Feed(g, Smooth(72), 9);
    EXPECT_EQ(g.Target(), 72);
}

TEST(RefreshRateGovernorTest, RisesAfterTenSmoothSecondsAtTheLowRate) {
    RefreshRateGovernor g;
    Feed(g, Smooth(90), 15);
    Feed(g, Late(60), 2);
    ASSERT_EQ(g.Target(), 72);
    Feed(g, Smooth(72), 9);
    EXPECT_EQ(g.Update(Smooth(72)), 90);
}

TEST(RefreshRateGovernorTest, WorkNearTheHighBudgetBlocksTheRise) {
    // At 72 Hz the frames are on time, but 10 ms of work leaves too little margin at 90 Hz.
    RefreshRateGovernor g;
    Feed(g, Smooth(90), 15);
    Feed(g, Late(60), 2);
    Feed(g, { 72.0f, 10.0f }, 30);
    EXPECT_EQ(g.Target(), 72);
}

TEST(RefreshRateGovernorTest, AverageWorkDecidesWithStereoDivisorTwo) {
    // Divisor 2: eye frames of 12 ms and light frames of 3 ms average 7.5 ms, which fits at 90 Hz.
    RefreshRateGovernor g;
    Feed(g, Smooth(90), 15);
    Feed(g, Late(60), 2);
    Feed(g, { 72.0f, 7.5f }, 10);
    EXPECT_EQ(g.Target(), 90);
}

TEST(RefreshRateGovernorTest, ALateSecondRestartsTheRiseCount) {
    RefreshRateGovernor g;
    Feed(g, Smooth(90), 15);
    Feed(g, Late(60), 2);
    Feed(g, Smooth(72), 8);
    g.Update({ 72.0f, 10.0f });
    Feed(g, Smooth(72), 9);
    EXPECT_EQ(g.Target(), 72);
    EXPECT_EQ(g.Update(Smooth(72)), 90);
}

TEST(RefreshRateGovernorTest, ResetGoesBackToTheHighRate) {
    RefreshRateGovernor g;
    Feed(g, Smooth(90), 15);
    Feed(g, Late(60), 2);
    ASSERT_EQ(g.Target(), 72);
    g.Reset();
    EXPECT_EQ(g.Target(), 90);
    // After a reset, the first seconds count from zero.
    Feed(g, Late(60), 1);
    EXPECT_EQ(g.Target(), 90);
}
