#include <gtest/gtest.h>
#include <memory>
#include "fast/frame_resource_cache.h"
#include "fast/vertex_ring.h"

// SOH [Quest] Pure helpers that decrease the CPU work of the renderer (issue #47).

TEST(VertexRingTest, FirstDrawStartsAtZero) {
    VertexRing ring(1024);
    const auto slot = ring.Reserve(96, 32);
    EXPECT_TRUE(slot.fits);
    EXPECT_FALSE(slot.wrapped);
    EXPECT_EQ(slot.offset, 0u);
}

TEST(VertexRingTest, NextDrawStartsAfterThePreviousDraw) {
    VertexRing ring(1024);
    ring.Reserve(96, 32);
    const auto slot = ring.Reserve(64, 32);
    EXPECT_FALSE(slot.wrapped);
    EXPECT_EQ(slot.offset, 96u);
}

TEST(VertexRingTest, OffsetIsAMultipleOfTheStride) {
    // The draw starts at vertex offset / stride, so the offset must be a full vertex.
    VertexRing ring(1024);
    ring.Reserve(100, 20);
    const auto slot = ring.Reserve(72, 24);
    EXPECT_EQ(slot.offset % 24, 0u);
    EXPECT_GE(slot.offset, 100u);
    EXPECT_EQ(slot.offset, 120u);
}

TEST(VertexRingTest, WrapsToZeroWhenTheDrawDoesNotFitAtTheEnd) {
    VertexRing ring(256);
    ring.Reserve(192, 32);
    const auto slot = ring.Reserve(96, 32);
    EXPECT_TRUE(slot.fits);
    EXPECT_TRUE(slot.wrapped);
    EXPECT_EQ(slot.offset, 0u);

    const auto next = ring.Reserve(32, 32);
    EXPECT_FALSE(next.wrapped);
    EXPECT_EQ(next.offset, 96u);
}

TEST(VertexRingTest, DrawThatFillsTheRingExactlyDoesNotWrap) {
    VertexRing ring(256);
    ring.Reserve(128, 32);
    const auto slot = ring.Reserve(128, 32);
    EXPECT_FALSE(slot.wrapped);
    EXPECT_EQ(slot.offset, 128u);
}

TEST(VertexRingTest, DrawLargerThanTheRingDoesNotFit) {
    VertexRing ring(256);
    const auto slot = ring.Reserve(512, 32);
    EXPECT_FALSE(slot.fits);
}

namespace {
struct FakeResource {
    int id;
};
} // namespace

TEST(FrameResourceCacheTest, LoadsEachHashOneTimeInAFrame) {
    FrameResourceCache<FakeResource> cache;
    int loads = 0;
    auto load = [&](uint64_t hash) {
        loads++;
        return std::make_shared<FakeResource>(FakeResource{ static_cast<int>(hash) });
    };

    EXPECT_EQ(cache.Get(7, load)->id, 7);
    EXPECT_EQ(cache.Get(7, load)->id, 7);
    EXPECT_EQ(cache.Get(9, load)->id, 9);
    EXPECT_EQ(loads, 2);
}

TEST(FrameResourceCacheTest, ClearMakesTheNextFrameLoadAgain) {
    FrameResourceCache<FakeResource> cache;
    int loads = 0;
    auto load = [&](uint64_t) {
        loads++;
        return std::make_shared<FakeResource>(FakeResource{ loads });
    };

    cache.Get(7, load);
    cache.Clear();
    EXPECT_EQ(cache.Get(7, load)->id, 2);
    EXPECT_EQ(loads, 2);
}

TEST(FrameResourceCacheTest, KeepsAMissingResourceMissingForTheFrame) {
    FrameResourceCache<FakeResource> cache;
    int loads = 0;
    auto load = [&](uint64_t) {
        loads++;
        return std::shared_ptr<FakeResource>();
    };

    EXPECT_EQ(cache.Get(7, load), nullptr);
    EXPECT_EQ(cache.Get(7, load), nullptr);
    EXPECT_EQ(loads, 1);
}

TEST(FrameResourceCacheTest, KeepsTheResourceAliveUntilClear) {
    FrameResourceCache<FakeResource> cache;
    std::weak_ptr<FakeResource> weak;
    cache.Get(7, [&](uint64_t) {
        auto res = std::make_shared<FakeResource>(FakeResource{ 7 });
        weak = res;
        return res;
    });
    EXPECT_FALSE(weak.expired());
    cache.Clear();
    EXPECT_TRUE(weak.expired());
}
