#include <gtest/gtest.h>
#include "fast/HostPointer.h"

using Fast::IsPlausibleHostPointer;

// Display-list words are either N64 segment addresses or real pointers into this process. The
// interpreter only dereferences the second kind, so the filter must reject the first without
// rejecting any address the platform can actually hand out.

TEST(HostPointerTest, RejectsNullAndLowSegmentAddresses) {
    EXPECT_FALSE(IsPlausibleHostPointer(0, false));
    EXPECT_FALSE(IsPlausibleHostPointer(0xFFFF, false));
    EXPECT_FALSE(IsPlausibleHostPointer(0, true));
}

TEST(HostPointerTest, AcceptsOrdinaryUserSpaceAddress) {
    EXPECT_TRUE(IsPlausibleHostPointer(0x10000, false));
    EXPECT_TRUE(IsPlausibleHostPointer(0x00007F12345678A0ull, false));
}

#if UINTPTR_MAX > 0xFFFFFFFFu
TEST(HostPointerTest, RejectsAddressesAbove48BitsWithoutTopByteIgnore) {
    EXPECT_FALSE(IsPlausibleHostPointer(0x0001000000000000ull, false));
    EXPECT_FALSE(IsPlausibleHostPointer(0xB400007A12345678ull, false));
}

// Android on arm64 tags heap pointers in the top byte (0xB4 by default from targetSdk 30), and the
// hardware ignores that byte on dereference. A malloc'd "__OTR__..." path must still count.
TEST(HostPointerTest, AcceptsTaggedHeapPointerWithTopByteIgnore) {
    EXPECT_TRUE(IsPlausibleHostPointer(0xB400007A12345678ull, true));
}

TEST(HostPointerTest, TopByteIgnoreStillRejectsBits48To55) {
    EXPECT_FALSE(IsPlausibleHostPointer(0xB401007A12345678ull, true));
}

TEST(HostPointerTest, TopByteIgnoreStillRejectsTaggedLowAddress) {
    EXPECT_FALSE(IsPlausibleHostPointer(0xB40000000000FFFFull, true));
}
#endif
