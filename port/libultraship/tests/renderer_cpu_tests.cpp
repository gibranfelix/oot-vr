#include <gtest/gtest.h>
#include <memory>
#include "fast/frame_resource_cache.h"

// SOH [Quest] Pure helpers that decrease the CPU work of the renderer (issue #47).

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


TEST(FrameResourceCacheTest, LoadsEachPathOneTimeInAFrame) {
    FrameResourceCache<FakeResource> cache;
    int loads = 0;
    auto load = [&](const char* path) {
        loads++;
        return std::make_shared<FakeResource>(FakeResource{ path[0] });
    };

    // Two different buffers with the same text are the same resource.
    char first[] = "__OTR__objects/gameplay_keep/gTex";
    char second[] = "__OTR__objects/gameplay_keep/gTex";
    EXPECT_EQ(cache.Get(first, load), cache.Get(second, load));
    cache.Get("__OTR__objects/other", load);
    EXPECT_EQ(loads, 2);
}

TEST(FrameResourceCacheTest, KeepsThePathAfterTheCallerChangesItsBuffer) {
    FrameResourceCache<FakeResource> cache;
    int loads = 0;
    auto load = [&](const char*) {
        loads++;
        return std::make_shared<FakeResource>(FakeResource{ loads });
    };

    char path[] = "__OTR__a";
    cache.Get(path, load);
    path[7] = 'b';
    EXPECT_EQ(cache.Get(path, load)->id, 2);
    EXPECT_EQ(cache.Get("__OTR__a", load)->id, 1);
}

TEST(FrameResourceCacheTest, CountsHitsAndLoads) {
    FrameResourceCache<FakeResource> cache;
    auto loadHash = [](uint64_t) { return std::make_shared<FakeResource>(FakeResource{ 0 }); };
    auto loadPath = [](const char*) { return std::make_shared<FakeResource>(FakeResource{ 0 }); };

    cache.Get(7, loadHash);
    cache.Get(7, loadHash);
    cache.Get("a", loadPath);
    cache.Get("a", loadPath);
    cache.Get("a", loadPath);
    EXPECT_EQ(cache.Hits(), 3u);
    EXPECT_EQ(cache.Loads(), 2u);

    // The counters do not reset at Clear: the caller reads the totals and takes the difference.
    cache.Clear();
    EXPECT_EQ(cache.Hits(), 3u);
}
