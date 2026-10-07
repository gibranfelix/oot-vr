#include <gtest/gtest.h>
#include "fast/stereo_raster.h"

// SOH [VR] Triangle rejection and face culling for one eye and for the two eyes of a multiview
// pass (issue #80).

using namespace StereoRaster;

namespace {
constexpr ClipPos kCenter = { 0.0f, 0.0f, 0.5f, 1.0f };
} // namespace

TEST(StereoRasterTest, VertexInsideTheViewHasNoRejectBits) {
    EXPECT_EQ(ClipRejectMask(kCenter), 0);
}

TEST(StereoRasterTest, VertexOutsideEachPlaneSetsItsBit) {
    EXPECT_EQ(ClipRejectMask({ -2.0f, 0.0f, 0.5f, 1.0f }), kClipLeft);
    EXPECT_EQ(ClipRejectMask({ 2.0f, 0.0f, 0.5f, 1.0f }), kClipRight);
    EXPECT_EQ(ClipRejectMask({ 0.0f, -2.0f, 0.5f, 1.0f }), kClipBottom);
    EXPECT_EQ(ClipRejectMask({ 0.0f, 2.0f, 0.5f, 1.0f }), kClipTop);
    EXPECT_EQ(ClipRejectMask({ 0.0f, 0.0f, 2.0f, 1.0f }), kClipFar);
    EXPECT_EQ(ClipRejectMask({ -2.0f, 2.0f, 0.5f, 1.0f }), kClipLeft | kClipTop);
}

TEST(StereoRasterTest, NearPlaneDoesNotReject) {
    EXPECT_EQ(ClipRejectMask({ 0.0f, 0.0f, -2.0f, 1.0f }), 0);
}

TEST(StereoRasterTest, StereoMaskKeepsAVertexThatOneEyeSees) {
    // Left of the left eye's view, but inside the right eye's view.
    const uint8_t left = ClipRejectMask({ -1.2f, 0.0f, 0.5f, 1.0f });
    const uint8_t right = ClipRejectMask({ -0.6f, 0.0f, 0.5f, 1.0f });
    EXPECT_EQ(StereoRejectMask(left, right), 0);
}

TEST(StereoRasterTest, StereoMaskRejectsWhatBothEyesReject) {
    const uint8_t left = ClipRejectMask({ -3.0f, 0.0f, 0.5f, 1.0f });
    const uint8_t right = ClipRejectMask({ -2.0f, 0.0f, 0.5f, 1.0f });
    EXPECT_EQ(StereoRejectMask(left, right), kClipLeft);
}

TEST(StereoRasterTest, CrossSignFollowsTheWinding) {
    // Same formula as the interpreter: (v1 - v2) x (v3 - v2).
    const ClipPos a = { 0.0f, 0.0f, 0.5f, 1.0f };
    const ClipPos b = { 1.0f, 0.0f, 0.5f, 1.0f };
    const ClipPos c = { 0.0f, 1.0f, 0.5f, 1.0f };
    EXPECT_LT(ScreenCross(a, b, c), 0.0f);
    EXPECT_GT(ScreenCross(a, c, b), 0.0f);
}

TEST(StereoRasterTest, OneVertexBehindTheEyeFlipsTheCross) {
    const ClipPos a = { 0.0f, 0.0f, 0.5f, 1.0f };
    const ClipPos b = { 1.0f, 0.0f, 0.5f, 1.0f };
    const ClipPos c = { 0.0f, 1.0f, 0.5f, 1.0f };
    const ClipPos cBehind = { 0.0f, -1.0f, 0.5f, -1.0f }; // same screen point as c, w < 0
    EXPECT_GT(ScreenCross(a, b, cBehind), 0.0f);
    EXPECT_LT(ScreenCross(a, b, c), 0.0f);
}

TEST(StereoRasterTest, CullModes) {
    EXPECT_FALSE(IsCulled(1.0f, CullMode::None));
    EXPECT_TRUE(IsCulled(1.0f, CullMode::Back));
    EXPECT_FALSE(IsCulled(-1.0f, CullMode::Back));
    EXPECT_TRUE(IsCulled(-1.0f, CullMode::Front));
    EXPECT_FALSE(IsCulled(1.0f, CullMode::Front));
    EXPECT_TRUE(IsCulled(0.0f, CullMode::Back));
    EXPECT_TRUE(IsCulled(0.0f, CullMode::Front));
    EXPECT_TRUE(IsCulled(1.0f, CullMode::Both));
}

TEST(StereoRasterTest, StereoKeepsAFaceThatOneEyeSeesFromTheFront) {
    // An edge-on face: the left eye sees its back, the right eye sees its front.
    EXPECT_FALSE(IsCulledStereo(1.0f, -1.0f, CullMode::Back));
    EXPECT_TRUE(IsCulledStereo(1.0f, 2.0f, CullMode::Back));
    EXPECT_FALSE(IsCulledStereo(-1.0f, -2.0f, CullMode::Back));
}
