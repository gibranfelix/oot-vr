#pragma once

#include <stdint.h>

// SOH [VR] Triangle rejection and face culling in clip space, for one eye or for the two eyes of a
// multiview pass (issue #80). In a multiview pass the interpreter runs one time and draws each
// triangle into the two eyes, so it may drop a triangle only when neither eye can see it. Pure
// logic: no OpenGL, so the host unit tests can drive it. The formulas are the ones that
// Interpreter::GfxSpVertex and GfxSpTri1 used for one eye.
namespace StereoRaster {

struct ClipPos {
    float x, y, z, w;
};

constexpr uint8_t kClipLeft = 1;
constexpr uint8_t kClipRight = 2;
constexpr uint8_t kClipBottom = 4;
constexpr uint8_t kClipTop = 8;
constexpr uint8_t kClipFar = 32;

// The planes that the vertex is outside of. The near plane is not tested.
inline uint8_t ClipRejectMask(const ClipPos& p) {
    uint8_t mask = 0;
    if (p.x < -p.w) {
        mask |= kClipLeft;
    }
    if (p.x > p.w) {
        mask |= kClipRight;
    }
    if (p.y < -p.w) {
        mask |= kClipBottom;
    }
    if (p.y > p.w) {
        mask |= kClipTop;
    }
    if (p.z > p.w) {
        mask |= kClipFar;
    }
    return mask;
}

// A triangle is outside the view when its three vertices share a plane bit. For two eyes, keep only
// the planes that both eyes reject, so a triangle that one eye sees stays.
inline uint8_t StereoRejectMask(uint8_t left, uint8_t right) {
    return left & right;
}

// (v1 - v2) x (v3 - v2) in screen space. The sign gives the winding.
inline float ScreenCross(const ClipPos& v1, const ClipPos& v2, const ClipPos& v3) {
    const float dx1 = v1.x / v1.w - v2.x / v2.w;
    const float dy1 = v1.y / v1.w - v2.y / v2.w;
    const float dx2 = v3.x / v3.w - v2.x / v2.w;
    const float dy2 = v3.y / v3.w - v2.y / v2.w;
    float cross = dx1 * dy2 - dy1 * dx2;
    if ((v1.w < 0) ^ (v2.w < 0) ^ (v3.w < 0)) {
        // If one vertex lies behind the eye, negating cross gives the correct result. If all
        // vertices lie behind the eye, the clip test rejects the triangle.
        cross = -cross;
    }
    return cross;
}

enum class CullMode { None, Front, Back, Both };

inline bool IsCulled(float cross, CullMode mode) {
    switch (mode) {
        case CullMode::Front:
            return cross <= 0;
        case CullMode::Back:
            return cross >= 0;
        case CullMode::Both:
            return true;
        case CullMode::None:
        default:
            return false;
    }
}

// Drop the face only when both eyes see the culled side.
inline bool IsCulledStereo(float crossLeft, float crossRight, CullMode mode) {
    return IsCulled(crossLeft, mode) && IsCulled(crossRight, mode);
}

} // namespace StereoRaster
