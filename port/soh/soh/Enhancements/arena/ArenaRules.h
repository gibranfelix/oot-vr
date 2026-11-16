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

// The shop starts after each third wave (issue #169).
constexpr int kShopEveryWaves = 3;

inline bool IsShopAfterWave(int wave) {
    return wave > 0 && wave % kShopEveryWaves == 0;
}

// The items of the shop (issue #169). The order is the order of the scrubs in the shop.
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

// What an item needs before the shop sells it.
enum class ShopNeed {
    Nothing,
    Magic,
    BombBag,
    NoShield,
};

struct ShopItemSpec {
    ShopItem item;
    // The first shop that sells the item.
    int firstShop;
    int price;
    // Link can buy a consumable again. The shop sells other items only when Link does not have them.
    bool consumable;
    ShopNeed need;
};

constexpr ShopItemSpec kShopItems[kShopItemCount] = {
    { ShopItem::RedPotion, 1, 40, true, ShopNeed::Nothing },
    { ShopItem::GreenPotion, 1, 30, true, ShopNeed::Magic },
    { ShopItem::RecoveryHearts, 1, 15, true, ShopNeed::Nothing },
    { ShopItem::Arrows, 1, 20, true, ShopNeed::Nothing },
    { ShopItem::Bombs, 1, 30, true, ShopNeed::BombBag },
    { ShopItem::HylianShield, 1, 30, true, ShopNeed::NoShield },
    // Price() uses kHeartContainerPrices.
    { ShopItem::HeartContainer, 1, 0, true, ShopNeed::Nothing },
    { ShopItem::BombBag, 1, 60, false, ShopNeed::Nothing },
    { ShopItem::MirrorShield, 1, 120, false, ShopNeed::Nothing },
    { ShopItem::DekuNuts, 1, 40, true, ShopNeed::Nothing },
    { ShopItem::MagicDinsFire, 2, 200, false, ShopNeed::Nothing },
    { ShopItem::Longshot, 2, 150, false, ShopNeed::Nothing },
    { ShopItem::MegatonHammer, 2, 200, false, ShopNeed::Nothing },
    { ShopItem::HoverBoots, 2, 80, false, ShopNeed::Nothing },
    { ShopItem::BiggoronSword, 3, 500, false, ShopNeed::Nothing },
    { ShopItem::FireArrows, 3, 150, false, ShopNeed::Magic },
    { ShopItem::IceArrows, 3, 150, false, ShopNeed::Magic },
    { ShopItem::LightArrows, 3, 150, false, ShopNeed::Magic },
    { ShopItem::NayrusLove, 3, 200, false, ShopNeed::Magic },
    { ShopItem::DoubleDefense, 3, 300, false, ShopNeed::Nothing },
    { ShopItem::DoubleMagic, 3, 150, false, ShopNeed::Magic },
};

// The Heart Container costs more in each shop.
constexpr int kHeartContainerPrices[] = { 100, 150, 200 };

// What Link has. owns[MagicDinsFire] is true when Link has magic.
struct LinkState {
    bool owns[kShopItemCount] = {};
    bool hasShield = true;
    int hearts = 0;
};

inline const ShopItemSpec& GetShopItem(ShopItem item) {
    return kShopItems[static_cast<int>(item)];
}

inline int Price(ShopItem item, int shop) {
    if (item == ShopItem::HeartContainer) {
        constexpr int count = sizeof(kHeartContainerPrices) / sizeof(kHeartContainerPrices[0]);
        return kHeartContainerPrices[(shop < 1 ? 1 : (shop > count ? count : shop)) - 1];
    }
    return GetShopItem(item).price;
}

// The shop has the item, but Link can need another item first.
inline bool IsInShop(const ShopItemSpec& spec, int shop, const LinkState& link) {
    return shop >= spec.firstShop && (spec.consumable || !link.owns[static_cast<int>(spec.item)]);
}

inline bool IsOffered(ShopItem item, int shop, const LinkState& link) {
    const ShopItemSpec& spec = GetShopItem(item);
    if (!IsInShop(spec, shop, link)) {
        return false;
    }
    if (item == ShopItem::HeartContainer && link.hearts >= kMaxHearts) {
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

// The items of the shop, in the order of kShopItems. An item that Link did not buy stays in the next shops.
inline std::vector<ShopItem> Offers(int shop, const LinkState& link) {
    std::vector<ShopItem> offers;
    for (const ShopItemSpec& spec : kShopItems) {
        if (IsOffered(spec.item, shop, link)) {
            offers.push_back(spec.item);
        }
    }
    return offers;
}

// The places of the scrubs for all the shop: the items of the shop, also when Link must first buy their need. A scrub
// comes to its place when the shop sells its item, so the scrubs do not move.
inline std::vector<ShopItem> ShopSlots(int shop, const LinkState& link) {
    std::vector<ShopItem> slots;
    for (const ShopItemSpec& spec : kShopItems) {
        if (IsInShop(spec, shop, link)) {
            slots.push_back(spec.item);
        }
    }
    return slots;
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

    // 1 for the shop after wave 3, 2 for the shop after wave 6, ...
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

// The scrubs of the shop stand on a ring around Link. A scrub talks to Link from 130 units (EnDns_Idle), so the ring
// is larger: Link walks to the scrub that he selects. The ring grows when there are many scrubs, and it moves to the
// center of the arena when it is not in the arena.
constexpr float kShopRingMinRadius = 130.0f;
constexpr float kScrubSpacing = 62.0f;

inline std::vector<Point> ShopPositions(Point center, float radius, Point link, int count) {
    constexpr float kTwoPi = 6.2831853f;
    float ring = count * kScrubSpacing / kTwoPi;
    if (ring < kShopRingMinRadius) {
        ring = kShopRingMinRadius;
    }
    if (ring > radius) {
        ring = radius;
    }
    Point anchor = KeepInside(center, radius - ring, link);

    std::vector<Point> points;
    for (int i = 0; i < count; i++) {
        float angle = kTwoPi * i / count;
        points.push_back(Point{ anchor.x + ring * std::cos(angle), anchor.z + ring * std::sin(angle) });
    }
    return points;
}

} // namespace ArenaRules
