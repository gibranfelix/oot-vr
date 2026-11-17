#pragma once

// Pure rules of ARENA (Arena.cpp): the curve of the waves, the wave director, the spawn points, the rupees, and the
// catalog of the shop (ArenaShop.cpp). No game types, so
// the host tests in port/soh/tests/arena/ArenaRulesTests.cpp can build it alone.

#include <cmath>
#include <vector>

namespace ArenaRules {

constexpr int kLevels = 4;
constexpr int kMaxAlive = 10;
// The game logic runs 20 times each second (R_UPDATE_RATE 3).
constexpr int kFramesPerSecond = 20;
// At the start, and when the scene loads again, the player needs time to get ready. Between waves the player is ready.
constexpr int kStartPauseFrames = 10 * kFramesPerSecond;
constexpr int kWavePauseFrames = 5 * kFramesPerSecond;

// The enemies of a wave: the number of each level, and how many of them are Hyper. Index 0 is level 1.
struct WaveSpec {
    int count[kLevels];
    int hyper[kLevels];

    int Total() const {
        return count[0] + count[1] + count[2] + count[3];
    }
};

// The curve of issue #115, one row for each wave. From wave 22, all waves are the last row.
constexpr WaveSpec kCurve[] = {
    { { 4, 0, 0, 0 }, { 0, 0, 0, 0 } },   // 1
    { { 5, 0, 0, 0 }, { 0, 0, 0, 0 } },   // 2
    { { 6, 0, 0, 0 }, { 0, 0, 0, 0 } },   // 3
    { { 4, 3, 0, 0 }, { 0, 0, 0, 0 } },   // 4
    { { 4, 4, 0, 0 }, { 0, 0, 0, 0 } },   // 5
    { { 3, 6, 0, 0 }, { 0, 0, 0, 0 } },   // 6
    { { 3, 4, 3, 0 }, { 1, 1, 0, 0 } },   // 7
    { { 3, 4, 4, 0 }, { 2, 1, 0, 0 } },   // 8
    { { 3, 4, 5, 0 }, { 2, 1, 1, 0 } },   // 9
    { { 3, 4, 5, 1 }, { 3, 2, 1, 0 } },   // 10
    { { 3, 5, 5, 1 }, { 3, 3, 2, 0 } },   // 11
    { { 3, 5, 6, 1 }, { 3, 3, 2, 0 } },   // 12
    { { 3, 5, 7, 1 }, { 3, 4, 3, 0 } },   // 13
    { { 3, 6, 7, 1 }, { 3, 5, 4, 1 } },   // 14
    { { 3, 6, 8, 1 }, { 3, 5, 6, 1 } },   // 15
    { { 3, 6, 8, 2 }, { 3, 6, 6, 1 } },   // 16
    { { 3, 6, 9, 2 }, { 3, 6, 8, 1 } },   // 17
    { { 3, 7, 9, 2 }, { 3, 7, 9, 1 } },   // 18
    { { 3, 7, 10, 2 }, { 3, 7, 10, 2 } }, // 19
    { { 3, 7, 11, 2 }, { 3, 7, 11, 2 } }, // 20
    { { 3, 8, 11, 2 }, { 3, 8, 11, 2 } }, // 21
    { { 3, 8, 12, 2 }, { 3, 8, 12, 2 } }, // 22 and more
};
constexpr int kCurveWaves = sizeof(kCurve) / sizeof(kCurve[0]);

inline WaveSpec GetWave(int wave) {
    if (wave < 1) {
        wave = 1;
    }
    return kCurve[(wave > kCurveWaves ? kCurveWaves : wave) - 1];
}

// The rupees for a dead wave enemy, for levels 1 to 4. A Hyper enemy gives 2 times the value.
constexpr int kKillRupees[kLevels] = { 5, 10, 20, 50 };

inline int KillReward(int level, bool hyper) {
    return kKillRupees[level] * (hyper ? 2 : 1);
}

// The shop starts after each wave (issue #169).
constexpr int kShopEveryWaves = 1;

inline bool IsShopAfterWave(int wave) {
    return wave > 0 && wave % kShopEveryWaves == 0;
}

// The items of the shop (issue #169).
enum class ShopItem {
    RedPotion,
    GreenPotion,
    RecoveryHearts,
    Arrows,
    Bombs,
    HylianShield,
    HeartContainer,
    BombBag,
    MirrorShield,
    DekuNuts,
    MagicDinsFire,
    Longshot,
    MegatonHammer,
    HoverBoots,
    BiggoronSword,
    FireArrows,
    IceArrows,
    LightArrows,
    NayrusLove,
    DoubleDefense,
    DoubleMagic,
    Count,
};
constexpr int kShopItemCount = static_cast<int>(ShopItem::Count);
constexpr int kMaxHearts = 20;

// A stall is a group of scrubs that sell one type of item. Link can buy the healing and the ammo again and again. Each
// gear item sells one time in each shop.
enum class ShopStall {
    Healing,
    Ammo,
    Gear,
};
constexpr int kStallCount = 3;

// What an item needs before the shop sells it.
enum class ShopNeed {
    Nothing,
    Magic,
    BombBag,
    NoShield,
};

struct ShopItemSpec {
    ShopItem item;
    ShopStall stall;
    // Gear only: the tier of the item. Tier 2 comes in shop 4, tier 3 in shop 7 (kTierFirstShop).
    int tier;
    // The price in the first shop, before Link buys the item.
    int price;
    ShopNeed need;
};

constexpr ShopItemSpec kShopItems[kShopItemCount] = {
    { ShopItem::RedPotion, ShopStall::Healing, 0, 40, ShopNeed::Nothing },
    { ShopItem::GreenPotion, ShopStall::Healing, 0, 30, ShopNeed::Magic },
    { ShopItem::RecoveryHearts, ShopStall::Healing, 0, 15, ShopNeed::Nothing },
    { ShopItem::Arrows, ShopStall::Ammo, 0, 20, ShopNeed::Nothing },
    { ShopItem::Bombs, ShopStall::Ammo, 0, 30, ShopNeed::BombBag },
    { ShopItem::HylianShield, ShopStall::Gear, 1, 30, ShopNeed::NoShield },
    { ShopItem::HeartContainer, ShopStall::Gear, 1, 100, ShopNeed::Nothing },
    { ShopItem::BombBag, ShopStall::Gear, 1, 60, ShopNeed::Nothing },
    { ShopItem::MirrorShield, ShopStall::Gear, 2, 120, ShopNeed::Nothing },
    { ShopItem::DekuNuts, ShopStall::Ammo, 0, 40, ShopNeed::Nothing },
    { ShopItem::MagicDinsFire, ShopStall::Gear, 2, 200, ShopNeed::Nothing },
    { ShopItem::Longshot, ShopStall::Gear, 2, 150, ShopNeed::Nothing },
    { ShopItem::MegatonHammer, ShopStall::Gear, 2, 200, ShopNeed::Nothing },
    { ShopItem::HoverBoots, ShopStall::Gear, 1, 80, ShopNeed::Nothing },
    { ShopItem::BiggoronSword, ShopStall::Gear, 3, 500, ShopNeed::Nothing },
    { ShopItem::FireArrows, ShopStall::Gear, 3, 150, ShopNeed::Magic },
    { ShopItem::IceArrows, ShopStall::Gear, 3, 150, ShopNeed::Magic },
    { ShopItem::LightArrows, ShopStall::Gear, 3, 150, ShopNeed::Magic },
    { ShopItem::NayrusLove, ShopStall::Gear, 3, 200, ShopNeed::Magic },
    { ShopItem::DoubleDefense, ShopStall::Gear, 3, 300, ShopNeed::Nothing },
    { ShopItem::DoubleMagic, ShopStall::Gear, 3, 150, ShopNeed::Magic },
};

// The first shop of each tier of gear. Shop N is the shop after wave N.
constexpr int kTierFirstShop[] = { 1, 4, 7 };
constexpr int kTierCount = sizeof(kTierFirstShop) / sizeof(kTierFirstShop[0]);

// What Link has. owns[MagicDinsFire] is true when Link has magic.
struct LinkState {
    bool owns[kShopItemCount] = {};
    bool hasShield = true;
    int hearts = 0;
};

inline const ShopItemSpec& GetShopItem(ShopItem item) {
    return kShopItems[static_cast<int>(item)];
}

inline bool IsGear(ShopItem item) {
    return GetShopItem(item).stall == ShopStall::Gear;
}

// The highest tier of gear in the shop.
inline int UnlockedTier(int shop) {
    int tier = 0;
    for (int i = 0; i < kTierCount; i++) {
        if (shop >= kTierFirstShop[i]) {
            tier = i + 1;
        }
    }
    return tier;
}

// The number of gear items at random in the shop: 1 in shops 1 to 3, 2 in shops 4 to 6, and 3 after.
inline int GearOfferCount(int shop) {
    return UnlockedTier(shop);
}

// Gear costs 10 % more in each shop. Healing and ammo cost 25 % more for each item that Link bought in the same shop.
// The prices are multiples of 5 rupees.
constexpr int kGearIncreasePerShop = 10;
constexpr int kSupplyIncreasePerBuy = 25;

inline int Price(ShopItem item, int shop, int boughtInShop) {
    const ShopItemSpec& spec = GetShopItem(item);
    int percent = 100;
    if (spec.stall == ShopStall::Gear) {
        percent += kGearIncreasePerShop * (shop > 1 ? shop - 1 : 0);
    } else {
        percent += kSupplyIncreasePerBuy * boughtInShop;
    }
    return (spec.price * percent + 250) / 500 * 5;
}

// Link can buy the item now: Link has its need, and Link does not have the gear.
inline bool IsOffered(ShopItem item, const LinkState& link) {
    const ShopItemSpec& spec = GetShopItem(item);
    if (item == ShopItem::HeartContainer && link.hearts >= kMaxHearts) {
        return false;
    }
    if (spec.stall == ShopStall::Gear && link.owns[static_cast<int>(item)]) {
        return false;
    }
    switch (spec.need) {
        case ShopNeed::Magic:
            return link.owns[static_cast<int>(ShopItem::MagicDinsFire)];
        case ShopNeed::BombBag:
            return link.owns[static_cast<int>(ShopItem::BombBag)];
        case ShopNeed::NoShield:
            return !link.hasShield;
        case ShopNeed::Nothing:
            break;
    }
    return true;
}

// The items of each stall of one shop.
struct ShopPlan {
    std::vector<ShopItem> stalls[kStallCount];

    const std::vector<ShopItem>& Stall(ShopStall stall) const {
        return stalls[static_cast<int>(stall)];
    }
};

// Healing and ammo: all the items of the stall. An item that needs another item also gets a scrub, because Link can
// buy the need in the same shop. Its scrub comes when Link has the need.
// Gear: GearOfferCount(shop) items at random from the unlocked tiers, that Link can buy now. An item of lastGear (the
// gear of the last shop) comes again only when there are not sufficient other items. Without a shield, the Hylian
// Shield also comes. random returns values in [0, 1).
template <typename Random>
ShopPlan PlanShop(int shop, const LinkState& link, const std::vector<ShopItem>& lastGear, Random& random) {
    ShopPlan plan;
    std::vector<ShopItem> fresh;
    std::vector<ShopItem> again;
    for (const ShopItemSpec& spec : kShopItems) {
        if (spec.stall != ShopStall::Gear) {
            plan.stalls[static_cast<int>(spec.stall)].push_back(spec.item);
            continue;
        }
        if (!IsOffered(spec.item, link)) {
            continue;
        }
        if (spec.item == ShopItem::HylianShield) {
            plan.stalls[static_cast<int>(ShopStall::Gear)].push_back(spec.item);
            continue;
        }
        if (spec.tier > UnlockedTier(shop)) {
            continue;
        }
        bool wasInLastShop = false;
        for (ShopItem last : lastGear) {
            wasInLastShop = wasInLastShop || last == spec.item;
        }
        (wasInLastShop ? again : fresh).push_back(spec.item);
    }

    std::vector<ShopItem>& gear = plan.stalls[static_cast<int>(ShopStall::Gear)];
    for (int i = 0; i < GearOfferCount(shop); i++) {
        std::vector<ShopItem>& pool = fresh.empty() ? again : fresh;
        if (pool.empty()) {
            break;
        }
        int index = static_cast<int>(random() * pool.size());
        index = index < static_cast<int>(pool.size()) ? index : static_cast<int>(pool.size()) - 1;
        gear.push_back(pool[index]);
        pool.erase(pool.begin() + index);
    }
    return plan;
}

// The rupees when all the enemies of a wave are dead.
inline int WaveBonus(int wave) {
    return 10 * wave;
}

struct SpawnRequest {
    int level; // 0 to 3
    bool hyper;
};

// Starts the waves, and tells the game when to spawn an enemy. The game calls Tick one time each game frame.
class Director {
  public:
    void Start() {
        StartAt(1);
    }

    // Starts at a later wave. For the tests.
    void StartAt(int wave) {
        mWave = wave;
        mInShop = false;
        BeginPause(kStartPauseFrames);
    }

    // The scene loaded again, and the enemies of the wave are gone. The wave starts again after a pause. In the shop,
    // the shop continues.
    void RestartWave() {
        if (!mInShop) {
            BeginPause(kStartPauseFrames);
        }
    }

    // The shop has no time limit. The player starts the next wave.
    bool InShop() const {
        return mInShop;
    }

    // Shop N is the shop after wave N.
    int ShopNumber() const {
        return (mWave - 1) / kShopEveryWaves;
    }

    void LeaveShop() {
        if (mInShop) {
            mInShop = false;
            BeginPause(kWavePauseFrames);
        }
    }

    int Wave() const {
        return mWave;
    }

    bool InPause() const {
        return mPauseFramesLeft > 0;
    }

    // Rounds up, so the countdown shows 10, 9, ..., 1.
    int PauseSecondsLeft() const {
        return (mPauseFramesLeft + kFramesPerSecond - 1) / kFramesPerSecond;
    }

    // The enemies of the wave that the player must kill: the enemies that are alive and the enemies to spawn.
    int EnemiesLeft(int alive) const {
        return Remaining() + alive;
    }

    // alive: the number of wave enemies that are alive now. Returns true and sets request when the game must spawn
    // an enemy in this frame. The game spawns a maximum of one enemy in each frame.
    bool Tick(int alive, SpawnRequest* request) {
        if (mInShop) {
            return false;
        }
        if (mPauseFramesLeft > 0) {
            mPauseFramesLeft--;
            if (mPauseFramesLeft == 0) {
                BeginWave();
            }
            return false;
        }

        if (Remaining() == 0) {
            if (alive == 0) {
                mInShop = IsShopAfterWave(mWave);
                mWave++;
                if (!mInShop) {
                    BeginPause(kWavePauseFrames);
                }
            }
            return false;
        }

        if (alive >= kMaxAlive) {
            return false;
        }

        *request = Next();
        return true;
    }

    // The game could not spawn the enemy of the request. The director asks for it again in a later frame.
    void Requeue(const SpawnRequest& request) {
        mPending[request.level]++;
        if (request.hyper) {
            mPendingHyper[request.level]++;
        }
    }

  private:
    void BeginPause(int frames) {
        mPauseFramesLeft = frames;
        for (int level = 0; level < kLevels; level++) {
            mPending[level] = 0;
            mPendingHyper[level] = 0;
        }
    }

    void BeginWave() {
        WaveSpec spec = GetWave(mWave);
        for (int level = 0; level < kLevels; level++) {
            mPending[level] = spec.count[level];
            mPendingHyper[level] = spec.hyper[level];
        }
        mNextLevel = 0;
    }

    int Remaining() const {
        return mPending[0] + mPending[1] + mPending[2] + mPending[3];
    }

    // Takes the levels in turn, so the enemies of each level come during all the wave. In a level, the normal enemies
    // come first and the Hyper enemies last.
    SpawnRequest Next() {
        while (mPending[mNextLevel] == 0) {
            mNextLevel = (mNextLevel + 1) % kLevels;
        }
        SpawnRequest request;
        request.level = mNextLevel;
        request.hyper = mPendingHyper[mNextLevel] == mPending[mNextLevel];
        mPending[mNextLevel]--;
        if (request.hyper) {
            mPendingHyper[mNextLevel]--;
        }
        mNextLevel = (mNextLevel + 1) % kLevels;
        return request;
    }

    int mWave = 0;
    bool mInShop = false;
    int mPauseFramesLeft = 0;
    int mPending[kLevels] = {};
    int mPendingHyper[kLevels] = {};
    int mNextLevel = 0;
};

// A point on the floor plane: x and z of the game.
struct Point {
    float x;
    float z;
};

inline float DistanceBetween(Point a, Point b) {
    return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.z - b.z) * (a.z - b.z));
}

// Selects a random point in the circle of the arena, minDistance to maxDistance from Link. First it tries points in
// the ring around Link. If no point of the ring is in the arena, it uses the point of the arena that is nearest to the
// ring. random returns values in [0, 1).
template <typename Random>
Point PickSpawnPoint(Point center, float radius, Point link, float minDistance, float maxDistance, Random& random) {
    constexpr int kTries = 16;
    constexpr float kTwoPi = 6.2831853f;

    for (int i = 0; i < kTries; i++) {
        float r = minDistance + (maxDistance - minDistance) * random();
        float angle = kTwoPi * random();
        Point point{ link.x + r * std::cos(angle), link.z + r * std::sin(angle) };
        if (DistanceBetween(point, center) <= radius) {
            return point;
        }
    }

    Point best = center;
    float bestError = -1.0f;
    for (int i = 0; i < kTries; i++) {
        // The square root gives the same density in all the circle.
        float r = radius * std::sqrt(random());
        float angle = kTwoPi * random();
        Point point{ center.x + r * std::cos(angle), center.z + r * std::sin(angle) };
        float distance = DistanceBetween(point, link);
        float error =
            distance < minDistance ? minDistance - distance : (distance > maxDistance ? distance - maxDistance : 0.0f);
        if (bestError < 0.0f || error < bestError) {
            best = point;
            bestError = error;
        }
    }
    return best;
}

// Moves a point that is outside the circle back to the edge of the circle.
inline Point KeepInside(Point center, float radius, Point point) {
    float distance = DistanceBetween(point, center);
    if (distance <= radius) {
        return point;
    }
    float scale = radius / distance;
    return Point{ center.x + (point.x - center.x) * scale, center.z + (point.z - center.z) * scale };
}

// The places of the scrubs. The scrub that starts the next wave stands alone in front of Link, near enough to talk. The
// stalls stand on a ring around Link: healing on the left of Link, gear behind Link, and ammo on the right of Link. The
// scrubs of a stall stand side by side on the ring. A scrub talks to Link from 130 units (EnDns_Idle), so Link walks to
// a stall. The ring moves to the center of the arena when it is not in the arena.
constexpr float kShopRingRadius = 200.0f;
constexpr float kStartScrubDistance = 110.0f;
constexpr float kScrubSpacing = 62.0f;

struct ShopLayout {
    Point start;
    std::vector<Point> stalls[kStallCount];
};

// facing: the yaw of Link in radians. The game yaw 0 looks to +z, and the left of Link is +x.
inline ShopLayout ShopPositions(Point center, float radius, Point link, float facing, const int counts[kStallCount]) {
    constexpr float kPi = 3.14159265f;
    // The angle of each stall from the front of Link, in the order of ShopStall.
    constexpr float kStallAngles[kStallCount] = { kPi / 2.0f, -kPi / 2.0f, kPi };

    float ring = kShopRingRadius < radius ? kShopRingRadius : radius;
    Point anchor = KeepInside(center, radius - ring, link);
    auto at = [&anchor, facing](float angle, float distance) {
        return Point{ anchor.x + distance * std::sin(facing + angle), anchor.z + distance * std::cos(facing + angle) };
    };

    ShopLayout layout;
    layout.start = at(0.0f, kStartScrubDistance < ring ? kStartScrubDistance : ring);
    float step = kScrubSpacing / ring;
    for (int stall = 0; stall < kStallCount; stall++) {
        int count = counts[stall];
        for (int i = 0; i < count; i++) {
            float offset = (i - (count - 1) / 2.0f) * step;
            layout.stalls[stall].push_back(at(kStallAngles[stall] + offset, ring));
        }
    }
    return layout;
}

} // namespace ArenaRules
