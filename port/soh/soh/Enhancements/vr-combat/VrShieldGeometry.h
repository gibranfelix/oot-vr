#pragma once

// Pure shield geometry for the physical shield (VrShield.cpp): no game types, no OpenXR, so the
// host tests in port/soh/tests/vr/VrShieldGeometryTests.cpp can build it alone.

#include <cmath>

namespace VrShieldGeometry {

struct Vec3 {
    float x, y, z;
};

inline float Dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// A symmetric trapezoid in the collider's own axes (W = width, H = height, N = normal): the
// center in each axis, the half height, and the half widths of the top and bottom edges.
struct TrapezoidFit {
    bool valid;
    float centerW, centerH, centerN;
    float halfH, halfWTop, halfWBottom;
};

// The smallest trapezoid of this shape that covers every point once projected onto the
// collider plane (axW, axH unit axes; axN = plane normal). Each half of the outline sets the
// width of its own edge, so a tapered shield keeps its taper; then both edges grow together
// until every point is inside, so a round shield's widest middle is never cut off. The plane
// sits at the middle of the points' depth. Invalid when the points do not span an area.
inline TrapezoidFit FitTrapezoid(const Vec3* pts, int count, const Vec3& axW, const Vec3& axH, const Vec3& axN) {
    TrapezoidFit f = {};
    if (count < 3) {
        return f;
    }
    float wMin = Dot(pts[0], axW), wMax = wMin;
    float hMin = Dot(pts[0], axH), hMax = hMin;
    float nMin = Dot(pts[0], axN), nMax = nMin;
    for (int i = 1; i < count; i++) {
        const float w = Dot(pts[i], axW), h = Dot(pts[i], axH), n = Dot(pts[i], axN);
        wMin = std::fmin(wMin, w);
        wMax = std::fmax(wMax, w);
        hMin = std::fmin(hMin, h);
        hMax = std::fmax(hMax, h);
        nMin = std::fmin(nMin, n);
        nMax = std::fmax(nMax, n);
    }
    f.centerW = (wMin + wMax) * 0.5f;
    f.centerH = (hMin + hMax) * 0.5f;
    f.centerN = (nMin + nMax) * 0.5f;
    f.halfH = (hMax - hMin) * 0.5f;
    const float halfWMax = (wMax - wMin) * 0.5f;
    if (f.halfH < 1e-3f || halfWMax < 1e-3f) {
        return f;
    }

    // t = 0 at the bottom edge, 1 at the top edge.
    float top = 0.0f, bottom = 0.0f;
    for (int i = 0; i < count; i++) {
        const float a = std::fabs(Dot(pts[i], axW) - f.centerW);
        const float t = (Dot(pts[i], axH) - hMin) / (2.0f * f.halfH);
        if (t >= 0.5f) {
            top = std::fmax(top, a);
        } else {
            bottom = std::fmax(bottom, a);
        }
    }
    top = std::fmax(top, 1e-3f);
    bottom = std::fmax(bottom, 1e-3f);
    float grow = 1.0f;
    for (int i = 0; i < count; i++) {
        const float a = std::fabs(Dot(pts[i], axW) - f.centerW);
        const float t = (Dot(pts[i], axH) - hMin) / (2.0f * f.halfH);
        const float edge = bottom + (top - bottom) * t;
        grow = std::fmax(grow, a / edge);
    }
    f.halfWTop = top * grow;
    f.halfWBottom = bottom * grow;
    f.valid = true;
    return f;
}

// Center and unit normal of a collider quad (vanilla zigzag order: 0 = bottom-left,
// 1 = bottom-right, 2 = top-left, 3 = top-right). The normal points AWAY from `chest`, so
// "front" is always the outward face, for any winding and any hand pose. False when the quad
// is degenerate.
inline bool QuadFrame(const Vec3 quad[4], const Vec3& chest, Vec3* outCenter, Vec3* outNormal) {
    const Vec3 c = { (quad[0].x + quad[1].x + quad[2].x + quad[3].x) * 0.25f,
                     (quad[0].y + quad[1].y + quad[2].y + quad[3].y) * 0.25f,
                     (quad[0].z + quad[1].z + quad[2].z + quad[3].z) * 0.25f };
    const Vec3 e1 = { quad[1].x - quad[0].x, quad[1].y - quad[0].y, quad[1].z - quad[0].z };
    const Vec3 e2 = { quad[2].x - quad[0].x, quad[2].y - quad[0].y, quad[2].z - quad[0].z };
    Vec3 n = { e1.y * e2.z - e1.z * e2.y, e1.z * e2.x - e1.x * e2.z, e1.x * e2.y - e1.y * e2.x };
    const float len = std::sqrt(Dot(n, n));
    if (len < 1e-3f) {
        return false;
    }
    n = { n.x / len, n.y / len, n.z / len };
    const Vec3 out = { c.x - chest.x, c.y - chest.y, c.z - chest.z };
    if (Dot(n, out) < 0.0f) {
        n = { -n.x, -n.y, -n.z };
    }
    *outCenter = c;
    *outNormal = n;
    return true;
}

// True when `point` is inside the facing cone: at most maxDeg between the shield's outward
// normal and the direction from the shield center to the point. 180 = any direction.
inline bool FacesPoint(const Vec3& center, const Vec3& normal, const Vec3& point, float maxDeg) {
    if (maxDeg >= 179.0f) {
        return true;
    }
    const Vec3 d = { point.x - center.x, point.y - center.y, point.z - center.z };
    const float len = std::sqrt(Dot(d, d));
    if (len < 1e-3f) {
        return true;
    }
    return Dot(normal, d) / len >= std::cos(maxDeg * (3.14159265f / 180.0f));
}

} // namespace VrShieldGeometry
