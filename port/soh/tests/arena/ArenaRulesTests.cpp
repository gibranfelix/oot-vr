// Host unit tests for ArenaRules.h (pure rules of ARENA, no game). The game build does not compile this file. Run it
// from the repository root:
//
//   g++ -std=c++17 -I port/soh/soh/Enhancements/arena port/soh/tests/arena/ArenaRulesTests.cpp -o /tmp/arena_tests
//   /tmp/arena_tests

#include "ArenaRules.h"

#include <cmath>
#include <cstdio>
#include <vector>

using namespace ArenaRules;

static int sFailures = 0;

#define EXPECT(cond)                                                       \
    do {                                                                   \
        if (!(cond)) {                                                     \
            std::printf("%s:%d: FAILED: %s\n", __FILE__, __LINE__, #cond); \
            sFailures++;                                                   \
        }                                                                  \
    } while (0)

// The curve of issue #115: { count, Hyper } for levels 1 to 4, wave 1 to 22.
static const int kIssueCurve[22][4][2] = {
    { { 4, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },   { { 5, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
    { { 6, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },   { { 4, 0 }, { 3, 0 }, { 0, 0 }, { 0, 0 } },
    { { 4, 0 }, { 4, 0 }, { 0, 0 }, { 0, 0 } },   { { 3, 0 }, { 6, 0 }, { 0, 0 }, { 0, 0 } },
    { { 3, 1 }, { 4, 1 }, { 3, 0 }, { 0, 0 } },   { { 3, 2 }, { 4, 1 }, { 4, 0 }, { 0, 0 } },
    { { 3, 2 }, { 4, 1 }, { 5, 1 }, { 0, 0 } },   { { 3, 3 }, { 4, 2 }, { 5, 1 }, { 1, 0 } },
    { { 3, 3 }, { 5, 3 }, { 5, 2 }, { 1, 0 } },   { { 3, 3 }, { 5, 3 }, { 6, 2 }, { 1, 0 } },
    { { 3, 3 }, { 5, 4 }, { 7, 3 }, { 1, 0 } },   { { 3, 3 }, { 6, 5 }, { 7, 4 }, { 1, 1 } },
    { { 3, 3 }, { 6, 5 }, { 8, 6 }, { 1, 1 } },   { { 3, 3 }, { 6, 6 }, { 8, 6 }, { 2, 1 } },
    { { 3, 3 }, { 6, 6 }, { 9, 8 }, { 2, 1 } },   { { 3, 3 }, { 7, 7 }, { 9, 9 }, { 2, 1 } },
    { { 3, 3 }, { 7, 7 }, { 10, 10 }, { 2, 2 } }, { { 3, 3 }, { 7, 7 }, { 11, 11 }, { 2, 2 } },
    { { 3, 3 }, { 8, 8 }, { 11, 11 }, { 2, 2 } }, { { 3, 3 }, { 8, 8 }, { 12, 12 }, { 2, 2 } },
};

static void CurveIsTheCurveOfTheIssue() {
    for (int wave = 1; wave <= 22; wave++) {
        WaveSpec spec = GetWave(wave);
        for (int level = 0; level < kLevels; level++) {
            EXPECT(spec.count[level] == kIssueCurve[wave - 1][level][0]);
            EXPECT(spec.hyper[level] == kIssueCurve[wave - 1][level][1]);
        }
    }
}

static void WaveHasThreePlusNEnemiesUpTo25() {
    for (int wave = 1; wave <= 40; wave++) {
        int expected = wave + 3 > 25 ? 25 : wave + 3;
        EXPECT(GetWave(wave).Total() == expected);
    }
}

static void WavesAfter22AreWave22() {
    WaveSpec last = GetWave(22);
    WaveSpec later = GetWave(500);
    for (int level = 0; level < kLevels; level++) {
        EXPECT(later.count[level] == last.count[level]);
        EXPECT(later.hyper[level] == last.hyper[level]);
    }
}

// Runs the director until it asks for no enemy. Counts the requests.
static int DrainSpawns(Director& director, int alive, int counts[kLevels], int hyper[kLevels]) {
    int total = 0;
    SpawnRequest request;
    while (director.Tick(alive, &request)) {
        counts[request.level]++;
        if (request.hyper) {
            hyper[request.level]++;
        }
        alive++;
        total++;
    }
    return total;
}

static void SkipPause(Director& director) {
    SpawnRequest request;
    int frames = 0;
    while (director.InPause() && frames <= kStartPauseFrames) {
        EXPECT(!director.Tick(0, &request));
        frames++;
    }
    EXPECT(!director.InPause());
}

static void FirstWaveStartsAfterThePause() {
    Director director;
    director.Start();
    EXPECT(director.Wave() == 1);
    SkipPause(director);
    EXPECT(!director.InPause());
}

static void PauseShowsTheSecondsLeft() {
    Director director;
    director.Start();
    // The player needs 10 seconds to get ready (headset test of issue #115).
    EXPECT(director.PauseSecondsLeft() == 10);
    SpawnRequest request;
    director.Tick(0, &request);
    EXPECT(director.PauseSecondsLeft() == 10);
    for (int i = 1; i < kStartPauseFrames - 1; i++) {
        director.Tick(0, &request);
    }
    EXPECT(director.PauseSecondsLeft() == 1);
}

// Between waves the player is ready, so the pause is shorter (headset test of issue #115).
static void PauseBetweenWavesIsFiveSeconds() {
    Director director;
    director.Start();
    SkipPause(director);
    int counts[kLevels] = {};
    int hyper[kLevels] = {};
    DrainSpawns(director, 0, counts, hyper);
    SpawnRequest request;
    EXPECT(!director.Tick(0, &request));
    EXPECT(director.Wave() == 2);
    // The shop opens after each wave. The pause starts when the player leaves the shop.
    EXPECT(director.InShop());
    director.LeaveShop();
    EXPECT(director.PauseSecondsLeft() == 5);
}

static void DirectorSpawnsTheWaveOfTheCurve() {
    Director director;
    director.Start();
    SkipPause(director);
    int counts[kLevels] = {};
    int hyper[kLevels] = {};
    // Wave 1 has 4 enemies, fewer than the maximum alive.
    EXPECT(DrainSpawns(director, 0, counts, hyper) == 4);
    EXPECT(counts[0] == 4);
    EXPECT(hyper[0] == 0);
}

static void DirectorKeepsTheMaximumAlive() {
    Director director;
    director.StartAt(20);
    SkipPause(director);
    int counts[kLevels] = {};
    int hyper[kLevels] = {};
    EXPECT(DrainSpawns(director, 0, counts, hyper) == kMaxAlive);
    // One enemy died: one more spawns.
    EXPECT(DrainSpawns(director, kMaxAlive - 1, counts, hyper) == 1);
}

static void DirectorSpawnsAllLevelsAndHyperOfTheWave() {
    Director director;
    director.StartAt(16);
    SkipPause(director);
    int counts[kLevels] = {};
    int hyper[kLevels] = {};
    // The enemies die as fast as they spawn.
    SpawnRequest request;
    while (director.Tick(0, &request)) {
        counts[request.level]++;
        hyper[request.level] += request.hyper ? 1 : 0;
    }
    WaveSpec spec = GetWave(16);
    for (int level = 0; level < kLevels; level++) {
        EXPECT(counts[level] == spec.count[level]);
        EXPECT(hyper[level] == spec.hyper[level]);
    }
}

static void NextWaveStartsWhenAllEnemiesAreDead() {
    Director director;
    director.Start();
    SkipPause(director);
    int counts[kLevels] = {};
    int hyper[kLevels] = {};
    DrainSpawns(director, 0, counts, hyper);
    SpawnRequest request;
    // An enemy is alive: the wave continues.
    EXPECT(!director.Tick(1, &request));
    EXPECT(director.Wave() == 1);
    EXPECT(!director.InPause());
    // All enemies are dead: the shop opens. After the shop, the pause before wave 2 starts.
    EXPECT(!director.Tick(0, &request));
    EXPECT(director.Wave() == 2);
    EXPECT(director.InShop());
    director.LeaveShop();
    EXPECT(director.InPause());
    SkipPause(director);
    EXPECT(DrainSpawns(director, 0, counts, hyper) == 5);
}

static void RequeueKeepsTheCountsOfTheWave() {
    Director director;
    director.StartAt(10);
    SkipPause(director);
    int counts[kLevels] = {};
    int hyper[kLevels] = {};
    SpawnRequest request;
    bool failOnce = true;
    while (director.Tick(0, &request)) {
        if (failOnce) {
            director.Requeue(request);
            failOnce = false;
            continue;
        }
        counts[request.level]++;
        hyper[request.level] += request.hyper ? 1 : 0;
    }
    WaveSpec spec = GetWave(10);
    for (int level = 0; level < kLevels; level++) {
        EXPECT(counts[level] == spec.count[level]);
        EXPECT(hyper[level] == spec.hyper[level]);
    }
}

static float Distance(Point a, Point b) {
    return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.z - b.z) * (a.z - b.z));
}

// A fixed sequence of values in [0, 1), so the tests do not depend on a seed.
struct SequenceRandom {
    const float* values;
    int length;
    int next = 0;
    float operator()() {
        float value = values[next % length];
        next++;
        return value;
    }
};

static void SpawnPointIsInTheArenaAndFarFromLink() {
    static const float kValues[] = { 0.1f, 0.9f, 0.5f, 0.3f, 0.7f, 0.2f, 0.95f, 0.05f, 0.6f, 0.4f };
    SequenceRandom random{ kValues, 10 };
    Point center{ 100.0f, -200.0f };
    Point link{ 100.0f, -200.0f };
    for (int i = 0; i < 50; i++) {
        Point point = PickSpawnPoint(center, 600.0f, link, 300.0f, 600.0f, random);
        EXPECT(Distance(point, center) <= 600.0f + 0.01f);
        EXPECT(Distance(point, link) >= 300.0f);
    }
}

static void SpawnPointIsNearestToTheRingWhenTheRingIsOutsideTheArena() {
    static const float kValues[] = { 0.5f, 0.0f, 0.5f, 0.5f };
    SequenceRandom random{ kValues, 4 };
    Point center{ 0.0f, 0.0f };
    Point link{ 0.0f, 0.0f };
    // The arena is smaller than the minimum distance: no point is far enough.
    Point point = PickSpawnPoint(center, 100.0f, link, 500.0f, 600.0f, random);
    EXPECT(Distance(point, center) <= 100.0f + 0.01f);
    EXPECT(Distance(point, link) > 50.0f);
}

// Link is at the edge of a large arena: the far side of the arena is more than the maximum distance from Link.
static void SpawnPointIsNotFartherThanTheMaximum() {
    static const float kValues[] = { 0.1f, 0.9f, 0.5f, 0.3f, 0.7f, 0.2f, 0.95f, 0.05f, 0.6f, 0.4f, 0.8f };
    SequenceRandom random{ kValues, 11 };
    Point center{ -200.0f, -200.0f };
    Point link{ 450.0f, -200.0f };
    for (int i = 0; i < 50; i++) {
        Point point = PickSpawnPoint(center, 650.0f, link, 300.0f, 600.0f, random);
        EXPECT(Distance(point, center) <= 650.0f + 0.01f);
        EXPECT(Distance(point, link) >= 300.0f - 0.01f);
        EXPECT(Distance(point, link) <= 600.0f + 0.01f);
    }
}

// The scene loads again (for example after a void-out): the actors of the wave are gone.
static void RestartWaveSpawnsTheWholeWaveAgain() {
    Director director;
    director.StartAt(3);
    SkipPause(director);
    SpawnRequest request;
    EXPECT(director.Tick(0, &request));
    EXPECT(director.Tick(1, &request));
    director.RestartWave();
    EXPECT(director.Wave() == 3);
    // The scene loaded again, so the player needs the time to get ready.
    EXPECT(director.PauseSecondsLeft() == 10);
    SkipPause(director);
    int counts[kLevels] = {};
    int hyper[kLevels] = {};
    EXPECT(DrainSpawns(director, 0, counts, hyper) == GetWave(3).Total());
}

static void PointOutsideTheBoundaryComesBack() {
    Point center{ -200.0f, -200.0f };
    Point inside = KeepInside(center, 700.0f, Point{ 0.0f, 0.0f });
    EXPECT(inside.x == 0.0f && inside.z == 0.0f);
    Point outside = KeepInside(center, 700.0f, Point{ 1000.0f, -200.0f });
    EXPECT(std::fabs(Distance(outside, center) - 700.0f) < 0.01f);
    EXPECT(std::fabs(outside.z - -200.0f) < 0.01f);
}

static void EnemiesLeftCountsTheAliveAndTheEnemiesToSpawn() {
    Director director;
    director.Start();
    SkipPause(director);
    // Wave 1 has 4 enemies.
    EXPECT(director.EnemiesLeft(0) == 4);
    SpawnRequest request;
    EXPECT(director.Tick(0, &request));
    EXPECT(director.Tick(1, &request));
    EXPECT(director.EnemiesLeft(2) == 4);
    // One of the two enemies is dead.
    EXPECT(director.EnemiesLeft(1) == 3);
}

// Issue #169: 5, 10, 20, and 50 rupees for levels 1 to 4. A Hyper enemy gives 2 times the value.
static void DeadEnemyGivesTheRupeesOfItsLevel() {
    EXPECT(KillReward(0, false) == 5);
    EXPECT(KillReward(1, false) == 10);
    EXPECT(KillReward(2, false) == 20);
    EXPECT(KillReward(3, false) == 50);
    EXPECT(KillReward(0, true) == 10);
    EXPECT(KillReward(3, true) == 100);
}

static void ClearedWaveGivesTenTimesTheWaveNumber() {
    EXPECT(WaveBonus(1) == 10);
    EXPECT(WaveBonus(3) == 30);
    EXPECT(WaveBonus(12) == 120);
}

// Spawns all the enemies of the wave, and kills them.
static void ClearWave(Director& director) {
    SkipPause(director);
    int counts[kLevels] = {};
    int hyper[kLevels] = {};
    DrainSpawns(director, 0, counts, hyper);
    SpawnRequest request;
    EXPECT(!director.Tick(0, &request));
}

static void ShopStartsAfterEachWave() {
    Director director;
    director.StartAt(1);
    ClearWave(director);
    EXPECT(director.InShop());
    EXPECT(!director.InPause());
    EXPECT(director.Wave() == 2);
    EXPECT(director.ShopNumber() == 1);

    director.StartAt(6);
    ClearWave(director);
    EXPECT(director.InShop());
    EXPECT(director.ShopNumber() == 6);
}

static void ShopHasNoTimeLimit() {
    Director director;
    director.StartAt(3);
    ClearWave(director);
    SpawnRequest request;
    for (int frame = 0; frame < 100 * kFramesPerSecond; frame++) {
        EXPECT(!director.Tick(0, &request));
    }
    EXPECT(director.InShop());
    EXPECT(director.Wave() == 4);
}

static void NextWaveStartsFiveSecondsAfterTheShop() {
    Director director;
    director.StartAt(3);
    ClearWave(director);
    director.LeaveShop();
    EXPECT(!director.InShop());
    EXPECT(director.InPause());
    EXPECT(director.PauseSecondsLeft() == 5);
    SkipPause(director);
    int counts[kLevels] = {};
    int hyper[kLevels] = {};
    EXPECT(DrainSpawns(director, 0, counts, hyper) == GetWave(4).Total());
}

// A void-out in the shop loads the scene again. The shop continues.
static void RestartWaveInTheShopKeepsTheShop() {
    Director director;
    director.StartAt(3);
    ClearWave(director);
    director.RestartWave();
    EXPECT(director.InShop());
    EXPECT(!director.InPause());
}

// Link at the start of the run: the Master Sword, the Hylian Shield, the bow, and 6 hearts.
static LinkState StartLink() {
    LinkState link;
    link.hasShield = true;
    link.hearts = 6;
    return link;
}

static bool Has(const std::vector<ShopItem>& items, ShopItem item) {
    for (ShopItem each : items) {
        if (each == item) {
            return true;
        }
    }
    return false;
}

// A fixed sequence of random values, so the tests give the same result each time.
struct TestRandom {
    unsigned state = 1;
    float operator()() {
        state = state * 1103515245u + 12345u;
        return static_cast<float>((state >> 8) & 0xFFFF) / 65536.0f;
    }
};

static ShopPlan Plan(int shop, const LinkState& link, const std::vector<ShopItem>& lastGear = {}) {
    TestRandom random;
    return PlanShop(shop, link, lastGear, random);
}

static void HealingAndAmmoHaveAllTheirItems() {
    ShopPlan plan = Plan(1, StartLink());
    const std::vector<ShopItem>& healing = plan.Stall(ShopStall::Healing);
    const std::vector<ShopItem>& ammo = plan.Stall(ShopStall::Ammo);
    EXPECT(healing.size() == 3);
    EXPECT(Has(healing, ShopItem::RedPotion));
    EXPECT(Has(healing, ShopItem::GreenPotion));
    EXPECT(Has(healing, ShopItem::RecoveryHearts));
    EXPECT(ammo.size() == 3);
    EXPECT(Has(ammo, ShopItem::Arrows));
    EXPECT(Has(ammo, ShopItem::Bombs));
    EXPECT(Has(ammo, ShopItem::DekuNuts));
}

// The scrubs of the Green Potion and the bombs come when Link has magic or the Bomb Bag.
static void HealingAndAmmoNeedTheirNeed() {
    LinkState link = StartLink();
    EXPECT(IsOffered(ShopItem::RedPotion, link));
    EXPECT(IsOffered(ShopItem::Arrows, link));
    EXPECT(!IsOffered(ShopItem::GreenPotion, link));
    EXPECT(!IsOffered(ShopItem::Bombs, link));
    link.owns[static_cast<int>(ShopItem::MagicDinsFire)] = true;
    link.owns[static_cast<int>(ShopItem::BombBag)] = true;
    EXPECT(IsOffered(ShopItem::GreenPotion, link));
    EXPECT(IsOffered(ShopItem::Bombs, link));
}

static void GearCountGrowsWithTheShop() {
    const int expected[] = { 1, 1, 1, 2, 2, 2, 3, 3, 3, 3 };
    for (int shop = 1; shop <= 10; shop++) {
        EXPECT(GearOfferCount(shop) == expected[shop - 1]);
        EXPECT(static_cast<int>(Plan(shop, StartLink()).Stall(ShopStall::Gear).size()) == expected[shop - 1]);
    }
}

static void GearComesFromTheUnlockedTiers() {
    for (int shop = 1; shop <= 9; shop++) {
        for (unsigned seed = 1; seed < 50; seed++) {
            TestRandom random;
            random.state = seed;
            LinkState link = StartLink();
            link.owns[static_cast<int>(ShopItem::MagicDinsFire)] = true;
            ShopPlan plan = PlanShop(shop, link, {}, random);
            for (ShopItem item : plan.Stall(ShopStall::Gear)) {
                EXPECT(IsGear(item));
                EXPECT(GetShopItem(item).tier <= UnlockedTier(shop));
            }
        }
    }
}

static void GearIsNotTheSameItemTwoTimes() {
    for (unsigned seed = 1; seed < 50; seed++) {
        TestRandom random;
        random.state = seed;
        std::vector<ShopItem> gear = PlanShop(9, StartLink(), {}, random).Stall(ShopStall::Gear);
        for (size_t i = 0; i < gear.size(); i++) {
            for (size_t j = i + 1; j < gear.size(); j++) {
                EXPECT(gear[i] != gear[j]);
            }
        }
    }
}

static void GearOfTheLastShopComesOnlyWhenNecessary() {
    LinkState link = StartLink();
    // Tier 1 without the Hylian Shield: the Heart Container, the Bomb Bag, and the Hover Boots.
    for (unsigned seed = 1; seed < 50; seed++) {
        TestRandom random;
        random.state = seed;
        std::vector<ShopItem> gear = PlanShop(2, link, { ShopItem::HeartContainer }, random).Stall(ShopStall::Gear);
        EXPECT(gear.size() == 1);
        EXPECT(!Has(gear, ShopItem::HeartContainer));
    }
    link.owns[static_cast<int>(ShopItem::BombBag)] = true;
    link.owns[static_cast<int>(ShopItem::HoverBoots)] = true;
    std::vector<ShopItem> gear = Plan(2, link, { ShopItem::HeartContainer }).Stall(ShopStall::Gear);
    EXPECT(gear.size() == 1);
    EXPECT(Has(gear, ShopItem::HeartContainer));
}

static void GearIsRandom() {
    bool seen[kShopItemCount] = {};
    for (unsigned seed = 1; seed < 200; seed++) {
        TestRandom random;
        random.state = seed;
        ShopPlan plan = PlanShop(1, StartLink(), {}, random);
        for (ShopItem item : plan.Stall(ShopStall::Gear)) {
            seen[static_cast<int>(item)] = true;
        }
    }
    EXPECT(seen[static_cast<int>(ShopItem::HeartContainer)]);
    EXPECT(seen[static_cast<int>(ShopItem::BombBag)]);
    EXPECT(seen[static_cast<int>(ShopItem::HoverBoots)]);
}

static void ShopDoesNotSellGearThatLinkHas() {
    LinkState link = StartLink();
    link.owns[static_cast<int>(ShopItem::MirrorShield)] = true;
    link.owns[static_cast<int>(ShopItem::Longshot)] = true;
    EXPECT(!IsOffered(ShopItem::MirrorShield, link));
    EXPECT(!IsOffered(ShopItem::Longshot, link));
    for (unsigned seed = 1; seed < 50; seed++) {
        TestRandom random;
        random.state = seed;
        std::vector<ShopItem> gear = PlanShop(9, link, {}, random).Stall(ShopStall::Gear);
        EXPECT(!Has(gear, ShopItem::MirrorShield));
        EXPECT(!Has(gear, ShopItem::Longshot));
    }
}

static void MagicItemsNeedMagic() {
    LinkState link = StartLink();
    const ShopItem magicItems[] = { ShopItem::FireArrows, ShopItem::IceArrows, ShopItem::LightArrows,
                                    ShopItem::NayrusLove, ShopItem::DoubleMagic };
    for (ShopItem item : magicItems) {
        EXPECT(!IsOffered(item, link));
    }
    link.owns[static_cast<int>(ShopItem::MagicDinsFire)] = true;
    for (ShopItem item : magicItems) {
        EXPECT(IsOffered(item, link));
    }
    EXPECT(!IsOffered(ShopItem::MagicDinsFire, link));
}

// A Like-Like can eat the Hylian Shield. Then each shop sells it, in addition to the gear at random.
static void HylianShieldComesWithoutAShield() {
    LinkState link = StartLink();
    EXPECT(!Has(Plan(1, link).Stall(ShopStall::Gear), ShopItem::HylianShield));
    link.hasShield = false;
    std::vector<ShopItem> gear = Plan(1, link).Stall(ShopStall::Gear);
    EXPECT(Has(gear, ShopItem::HylianShield));
    EXPECT(gear.size() == 2);
}

static void HeartContainerStopsAtTwentyHearts() {
    LinkState link = StartLink();
    link.hearts = 19;
    EXPECT(IsOffered(ShopItem::HeartContainer, link));
    link.hearts = 20;
    EXPECT(!IsOffered(ShopItem::HeartContainer, link));
}

static void GearCostsTenPercentMoreInEachShop() {
    EXPECT(Price(ShopItem::HeartContainer, 1, 0) == 100);
    EXPECT(Price(ShopItem::HeartContainer, 4, 0) == 130);
    EXPECT(Price(ShopItem::HeartContainer, 10, 0) == 190);
    EXPECT(Price(ShopItem::MirrorShield, 4, 0) == 155);
    EXPECT(Price(ShopItem::BiggoronSword, 7, 0) == 800);
    // Gear sells one time in each shop: the purchases do not change its price.
    EXPECT(Price(ShopItem::HeartContainer, 1, 3) == 100);
}

static void SuppliesCostMoreAfterEachPurchaseInTheShop() {
    EXPECT(Price(ShopItem::RedPotion, 1, 0) == 40);
    EXPECT(Price(ShopItem::RedPotion, 1, 1) == 50);
    EXPECT(Price(ShopItem::RedPotion, 1, 2) == 60);
    EXPECT(Price(ShopItem::RedPotion, 9, 0) == 40);
    EXPECT(Price(ShopItem::RecoveryHearts, 1, 1) == 20);
    EXPECT(Price(ShopItem::Arrows, 5, 2) == 30);
}

static void PricesAreMultiplesOfFive() {
    for (int i = 0; i < kShopItemCount; i++) {
        for (int shop = 1; shop <= 25; shop++) {
            for (int bought = 0; bought < 5; bought++) {
                EXPECT(Price(static_cast<ShopItem>(i), shop, bought) % 5 == 0);
            }
        }
    }
}

static void EachItemHasItsRowInTheTable() {
    for (int i = 0; i < kShopItemCount; i++) {
        EXPECT(static_cast<int>(kShopItems[i].item) == i);
        EXPECT((kShopItems[i].stall == ShopStall::Gear) == (kShopItems[i].tier > 0));
    }
}

static void ScrubsAreInTheArenaAndNotOnTopOfEachOther() {
    const Point center{ 0.0f, 0.0f };
    const float radii[] = { 240.0f, 650.0f };
    const Point links[] = { { 0.0f, 0.0f }, { 230.0f, 0.0f }, { -100.0f, 150.0f } };
    const float facings[] = { 0.0f, 1.0f, 3.14f, -2.0f };
    // The largest shop: 3 healing, 3 ammo, 3 gear and the Hylian Shield.
    const int counts[kStallCount] = { 3, 3, 4 };
    for (float radius : radii) {
        for (Point link : links) {
            for (float facing : facings) {
                ShopLayout layout = ShopPositions(center, radius, link, facing, counts);
                std::vector<Point> points = { layout.start };
                for (int stall = 0; stall < kStallCount; stall++) {
                    EXPECT(static_cast<int>(layout.stalls[stall].size()) == counts[stall]);
                    points.insert(points.end(), layout.stalls[stall].begin(), layout.stalls[stall].end());
                }
                for (size_t i = 0; i < points.size(); i++) {
                    EXPECT(DistanceBetween(points[i], center) <= radius + 0.01f);
                    for (size_t j = i + 1; j < points.size(); j++) {
                        EXPECT(DistanceBetween(points[i], points[j]) >= 55.0f);
                    }
                }
            }
        }
    }
}

// Link at the center looks to +z. The left of Link is +x.
static void StartScrubIsInFrontAndStallsAreAroundLink() {
    const Point center{ 0.0f, 0.0f };
    const int counts[kStallCount] = { 1, 1, 1 };
    ShopLayout layout = ShopPositions(center, 650.0f, center, 0.0f, counts);
    EXPECT(layout.start.z > 100.0f && std::fabs(layout.start.x) < 1.0f);
    // A scrub talks to Link from 130 units: Link can talk to the start scrub without a step.
    EXPECT(DistanceBetween(layout.start, center) < 130.0f);
    Point healing = layout.stalls[static_cast<int>(ShopStall::Healing)][0];
    Point ammo = layout.stalls[static_cast<int>(ShopStall::Ammo)][0];
    Point gear = layout.stalls[static_cast<int>(ShopStall::Gear)][0];
    EXPECT(healing.x > 150.0f && std::fabs(healing.z) < 1.0f);
    EXPECT(ammo.x < -150.0f && std::fabs(ammo.z) < 1.0f);
    EXPECT(gear.z < -150.0f && std::fabs(gear.x) < 1.0f);
}

int main() {
    CurveIsTheCurveOfTheIssue();
    WaveHasThreePlusNEnemiesUpTo25();
    WavesAfter22AreWave22();
    FirstWaveStartsAfterThePause();
    PauseShowsTheSecondsLeft();
    PauseBetweenWavesIsFiveSeconds();
    DirectorSpawnsTheWaveOfTheCurve();
    DirectorKeepsTheMaximumAlive();
    DirectorSpawnsAllLevelsAndHyperOfTheWave();
    NextWaveStartsWhenAllEnemiesAreDead();
    RequeueKeepsTheCountsOfTheWave();
    SpawnPointIsInTheArenaAndFarFromLink();
    SpawnPointIsNearestToTheRingWhenTheRingIsOutsideTheArena();
    SpawnPointIsNotFartherThanTheMaximum();
    RestartWaveSpawnsTheWholeWaveAgain();
    PointOutsideTheBoundaryComesBack();
    EnemiesLeftCountsTheAliveAndTheEnemiesToSpawn();
    DeadEnemyGivesTheRupeesOfItsLevel();
    ClearedWaveGivesTenTimesTheWaveNumber();
    ShopStartsAfterEachWave();
    ShopHasNoTimeLimit();
    NextWaveStartsFiveSecondsAfterTheShop();
    RestartWaveInTheShopKeepsTheShop();
    HealingAndAmmoHaveAllTheirItems();
    HealingAndAmmoNeedTheirNeed();
    GearCountGrowsWithTheShop();
    GearComesFromTheUnlockedTiers();
    GearIsNotTheSameItemTwoTimes();
    GearOfTheLastShopComesOnlyWhenNecessary();
    GearIsRandom();
    ShopDoesNotSellGearThatLinkHas();
    MagicItemsNeedMagic();
    HylianShieldComesWithoutAShield();
    HeartContainerStopsAtTwentyHearts();
    GearCostsTenPercentMoreInEachShop();
    SuppliesCostMoreAfterEachPurchaseInTheShop();
    PricesAreMultiplesOfFive();
    EachItemHasItsRowInTheTable();
    ScrubsAreInTheArenaAndNotOnTopOfEachOther();
    StartScrubIsInFrontAndStallsAreAroundLink();
    if (sFailures == 0) {
        std::printf("All ArenaRules tests passed.\n");
    }
    return sFailures == 0 ? 0 : 1;
}
