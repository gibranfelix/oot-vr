#pragma once

#include <cstdint>

namespace Fast {

// Whether the top byte of a pointer is ignored on dereference (Top Byte Ignore). Android turns it
// on for arm64 and tags heap pointers there - 0xB4 in the top byte by default from targetSdk 30.
#if defined(__aarch64__)
inline constexpr bool kTopByteIgnored = true;
#else
inline constexpr bool kTopByteIgnored = false;
#endif

// Whether a display-list word can be a pointer into this process rather than an N64 segment
// address or a sentinel. Upper bound covers every user-space layout (x86_64 47-bit canonical,
// arm64 48-bit VA). With Top Byte Ignore the tag is stripped first: otherwise every "__OTR__..."
// path the game copies into a malloc'd buffer at runtime (font glyphs, the textbox background) is
// rejected and its ASCII gets decoded as texels.
inline bool IsPlausibleHostPointer(uintptr_t p, bool topByteIgnored = kTopByteIgnored) {
#if UINTPTR_MAX > 0xFFFFFFFFu
    if (topByteIgnored) {
        p &= 0x00FFFFFFFFFFFFFFull;
    }
    if (p > 0x0000FFFFFFFFFFFFull) {
        return false;
    }
#else
    (void)topByteIgnored;
#endif
    return p >= 0x10000;
}

} // namespace Fast
