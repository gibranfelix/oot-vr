#include "ArenaShop.h"

#include "soh/Enhancements/custom-message/CustomMessageManager.h"
#include "soh/frame_interpolation.h"

#include <array>
#include <cmath>
#include <string>
#include <spdlog/spdlog.h>

extern "C" {
#include <z64.h>
#include "functions.h"
#include "macros.h"
#include "variables.h"
#include "src/overlays/actors/ovl_En_Dns/z_en_dns.h"

extern PlayState* gPlayState;
extern SaveContext gSaveContext;

// Actions of z_en_dns.c. The shop kills only an idle scrub, so a scrub that sold can go into the floor.
void EnDns_Idle(EnDns* enDns, PlayState* play);
void EnDns_SetupBurrow(EnDns* enDns, PlayState* play);
void EnDns_SetupNoSaleBurrow(EnDns* enDns, PlayState* play);
void EnDns_Burrow(EnDns* enDns, PlayState* play);
void EnDns_PostBurrow(EnDns* enDns, PlayState* play);

// Randomizer model (draw.cpp).
void Randomizer_DrawDoubleDefense(PlayState* play, GetItemEntry* getItemEntry);
}

using ArenaRules::ShopItem;

// kItemScale: the scale of the shop items (EnGirlA).
static constexpr f32 kItemHeight = 70.0f;
static constexpr f32 kItemScale = 0.25f;

// Double Defense has no GID. DrawShopItem uses the randomizer model.
static constexpr s16 kItemDrawIds[ArenaRules::kShopItemCount] = {
    GID_POTION_RED,      GID_POTION_GREEN, GID_HEART,         GID_ARROWS_MEDIUM, GID_BOMB,      GID_SHIELD_HYLIAN,
    GID_HEART_CONTAINER, GID_BOMB_BAG_20,  GID_SHIELD_MIRROR, GID_NUTS,          GID_DINS_FIRE, GID_LONGSHOT,
    GID_HAMMER,          GID_BOOTS_HOVER,  GID_SWORD_BGS,     GID_ARROW_FIRE,    GID_ARROW_ICE, GID_ARROW_LIGHT,
    GID_NAYRUS_LOVE,     GID_MAXIMUM,      GID_MAGIC_LARGE,
};

static bool GoesIntoTheFloor(EnDns* enDns) {
    return enDns->actionFunc == EnDns_SetupBurrow || enDns->actionFunc == EnDns_SetupNoSaleBurrow ||
           enDns->actionFunc == EnDns_Burrow || enDns->actionFunc == EnDns_PostBurrow;
}

// Global scope: OPEN_DISPS declares FrameInterpolation_RecordOpenChild in the block, and it needs C linkage.
static void DrawShopItem(PlayState* play, Actor* scrub, ShopItem item) {
    // Turns and goes up and down.
    f32 frames = static_cast<f32>(play->gameplayFrames);
    Vec3f pos = { scrub->world.pos.x, scrub->world.pos.y + kItemHeight + 3.0f * sinf(frames * 0.1f),
                  scrub->world.pos.z };

    OPEN_DISPS(play->state.gfxCtx);
    // Scene light at the item, as in Actor_Draw.
    Lights* lights = LightContext_NewLights(&play->lightCtx, play->state.gfxCtx);
    Lights_BindAll(lights, play->lightCtx.listHead, &pos);
    Lights_Draw(lights, play->state.gfxCtx);
    CLOSE_DISPS(play->state.gfxCtx);

    Matrix_Push();
    Matrix_Translate(pos.x, pos.y, pos.z, MTXMODE_NEW);
    Matrix_RotateY(frames * 0.05f, MTXMODE_APPLY);
    Matrix_Scale(kItemScale, kItemScale, kItemScale, MTXMODE_APPLY);
    if (item == ShopItem::DoubleDefense) {
        Randomizer_DrawDoubleDefense(play, nullptr);
    } else {
        GetItem_Draw(play, kItemDrawIds[static_cast<int>(item)]);
    }
    Matrix_Pop();
}

namespace ArenaShop {
namespace {

// Text IDs that the game does not use.
constexpr u16 kItemTextId = 0x9400;
constexpr u16 kStartTextId = 0x9401;
// Yes/No question of the scrubs (z_en_dns.c).
constexpr u16 kBuyQuestionTextId = 0x10A3;

// 1.5 s.
constexpr int kRespawnFrames = 30;

// EnDns_Init uses 0.01.
constexpr f32 kStartScrubScale = 0.015f;

struct ItemText {
    ShopItem item;
    const char* english;
    const char* german;
    const char* french;
};

constexpr ItemText kItemTexts[ArenaRules::kShopItemCount] = {
    { ShopItem::RedPotion, "Red Potion", "Rotes Elixier", "Potion rouge" },
    { ShopItem::GreenPotion, "Green Potion", "Grünes Elixier", "Potion verte" },
    { ShopItem::RecoveryHearts, "3 Hearts", "3 Herzen", "3 cœurs" },
    { ShopItem::Arrows, "10 Arrows", "10 Pfeile", "10 flèches" },
    { ShopItem::Bombs, "10 Bombs", "10 Bomben", "10 bombes" },
    { ShopItem::HylianShield, "Hylian Shield", "Hylia-Schild", "Bouclier hylien" },
    { ShopItem::HeartContainer, "Heart Container", "Herzcontainer", "Réceptacle de cœur" },
    { ShopItem::BombBag, "Bomb Bag", "Bombentasche", "Sac de bombes" },
    { ShopItem::MirrorShield, "Mirror Shield", "Spiegelschild", "Bouclier miroir" },
    { ShopItem::DekuNuts, "10 Deku Nuts", "10 Deku-Nüsse", "10 noix Mojo" },
    { ShopItem::MagicDinsFire, "Magic + Din's Fire", "Magie + Dins Feuerinferno", "Magie + Feu de Din" },
    { ShopItem::Longshot, "Longshot", "Enterhaken", "Super grappin" },
    { ShopItem::MegatonHammer, "Megaton Hammer", "Stahlhammer", "Masse des Titans" },
    { ShopItem::HoverBoots, "Hover Boots", "Gleitstiefel", "Bottes des airs" },
    { ShopItem::BiggoronSword, "Biggoron's Sword", "Biggoron-Schwert", "Épée de Biggoron" },
    { ShopItem::FireArrows, "Fire Arrows", "Feuerpfeile", "Flèches de feu" },
    { ShopItem::IceArrows, "Ice Arrows", "Eispfeile", "Flèches de glace" },
    { ShopItem::LightArrows, "Light Arrows", "Lichtpfeile", "Flèches de lumière" },
    { ShopItem::NayrusLove, "Nayru's Love", "Nayrus Umarmung", "Amour de Nayru" },
    { ShopItem::DoubleDefense, "Double Defense", "Doppelte Verteidigung", "Double défense" },
    { ShopItem::DoubleMagic, "Double Magic", "Doppelte Magie", "Double magie" },
};

constexpr bool TextsAreInTheOrderOfTheItems() {
    for (int i = 0; i < ArenaRules::kShopItemCount; i++) {
        if (static_cast<int>(kItemTexts[i].item) != i) {
            return false;
        }
    }
    return true;
}
static_assert(TextsAreInTheOrderOfTheItems(), "kItemTexts must have one row for each ShopItem, in the same order");

struct Slot {
    // The scrub that starts the wave has no item.
    bool startsWave;
    ShopItem item;
    Place place;
    Actor* scrub;
    int respawnFramesLeft;
    // Purchases in this shop.
    int bought;
};

bool sOpen = false;
int sShop = 0;
int sNextWave = 0;
void (*sStartWave)() = nullptr;
std::vector<Slot> sSlots;
// The next shop sells other gear first.
std::vector<ShopItem> sLastGear;
// EnDns keeps a pointer to its entry.
std::array<DnsItemEntry, ArenaRules::kShopItemCount + 1> sEntries;

Slot* FindSlot(const Actor* actor) {
    for (Slot& slot : sSlots) {
        if (slot.scrub == actor && actor != nullptr) {
            return &slot;
        }
    }
    return nullptr;
}

// Gear sells one time in each shop.
bool IsSold(const Slot& slot, const ArenaRules::LinkState& link) {
    return slot.startsWave ||
           (ArenaRules::IsOffered(slot.item, link) && (!ArenaRules::IsGear(slot.item) || slot.bought == 0));
}

int SlotPrice(const Slot& slot) {
    return slot.startsWave ? 0 : ArenaRules::Price(slot.item, sShop, slot.bought);
}

u32 CanBuy(EnDns* enDns) {
    Slot* slot = FindSlot(&enDns->actor);
    if (slot == nullptr) {
        return DNS_CANBUY_RESULT_CANT_GET_NOW;
    }
    if (slot->startsWave) {
        return DNS_CANBUY_RESULT_SUCCESS;
    }
    if (!IsSold(*slot, CurrentLink())) {
        return DNS_CANBUY_RESULT_CANT_GET_NOW;
    }

    bool full = false;
    switch (slot->item) {
        case ShopItem::RedPotion:
        case ShopItem::RecoveryHearts:
            full = gSaveContext.health >= gSaveContext.healthCapacity;
            break;
        case ShopItem::GreenPotion:
            full = gSaveContext.magic >= gSaveContext.magicCapacity;
            break;
        case ShopItem::Arrows:
            full = AMMO(ITEM_BOW) >= CUR_CAPACITY(UPG_QUIVER);
            break;
        case ShopItem::Bombs:
            full = AMMO(ITEM_BOMB) >= CUR_CAPACITY(UPG_BOMB_BAG);
            break;
        case ShopItem::DekuNuts:
            full = CUR_CAPACITY(UPG_NUTS) != 0 && AMMO(ITEM_NUT) >= CUR_CAPACITY(UPG_NUTS);
            break;
        default:
            break;
    }
    if (full) {
        return DNS_CANBUY_RESULT_CAPACITY_FULL;
    }
    if (gSaveContext.rupees < enDns->dnsItemEntry->itemPrice) {
        return DNS_CANBUY_RESULT_NEED_RUPEES;
    }
    return DNS_CANBUY_RESULT_SUCCESS;
}

void EquipShield(PlayState* play, u16 value) {
    Inventory_ChangeEquipment(EQUIP_TYPE_SHIELD, value);
    Player_SetEquipmentData(play, GET_PLAYER(play));
}

// No get item animation: the shop continues.
void GiveItem(PlayState* play, ShopItem item) {
    switch (item) {
        case ShopItem::RedPotion:
            gSaveContext.healthAccumulator = MAX_HEALTH;
            break;
        case ShopItem::GreenPotion:
            Magic_Fill(play);
            break;
        case ShopItem::RecoveryHearts:
            Health_ChangeBy(play, 3 * 16);
            break;
        case ShopItem::Arrows:
            Item_Give(play, ITEM_ARROWS_MEDIUM);
            break;
        case ShopItem::Bombs:
            Item_Give(play, ITEM_BOMBS_10);
            break;
        case ShopItem::HylianShield:
            Item_Give(play, ITEM_SHIELD_HYLIAN);
            EquipShield(play, EQUIP_VALUE_SHIELD_HYLIAN);
            break;
        case ShopItem::HeartContainer:
            Item_Give(play, ITEM_HEART_CONTAINER);
            break;
        case ShopItem::BombBag:
            Item_Give(play, ITEM_BOMB_BAG_20);
            break;
        case ShopItem::MirrorShield:
            Item_Give(play, ITEM_SHIELD_MIRROR);
            EquipShield(play, EQUIP_VALUE_SHIELD_MIRROR);
            break;
        case ShopItem::DekuNuts:
            Item_Give(play, ITEM_NUTS_10);
            break;
        case ShopItem::MagicDinsFire:
            // The same as the Great Fairy (z_bg_dy_yoseizo.c). Interface_Update fills the meter.
            gSaveContext.isMagicAcquired = true;
            gSaveContext.magicFillTarget = MAGIC_NORMAL_METER;
            gSaveContext.magicLevel = 0;
            Item_Give(play, ITEM_DINS_FIRE);
            break;
        case ShopItem::Longshot:
            Item_Give(play, ITEM_LONGSHOT);
            break;
        case ShopItem::MegatonHammer:
            Item_Give(play, ITEM_HAMMER);
            break;
        case ShopItem::HoverBoots:
            Item_Give(play, ITEM_BOOTS_HOVER);
            break;
        case ShopItem::BiggoronSword:
            Item_Give(play, ITEM_SWORD_BGS);
            gSaveContext.bgsFlag = true;
            break;
        case ShopItem::FireArrows:
            Item_Give(play, ITEM_ARROW_FIRE);
            break;
        case ShopItem::IceArrows:
            Item_Give(play, ITEM_ARROW_ICE);
            break;
        case ShopItem::LightArrows:
            Item_Give(play, ITEM_ARROW_LIGHT);
            break;
        case ShopItem::NayrusLove:
            Item_Give(play, ITEM_NAYRUS_LOVE);
            break;
        case ShopItem::DoubleDefense:
            // The same as the randomizer (randomizer.cpp, RG_DOUBLE_DEFENSE).
            gSaveContext.isDoubleDefenseAcquired = true;
            gSaveContext.inventory.defenseHearts = 20;
            break;
        case ShopItem::DoubleMagic:
            gSaveContext.isMagicAcquired = true;
            gSaveContext.isDoubleMagicAcquired = true;
            gSaveContext.magicFillTarget = MAGIC_DOUBLE_METER;
            gSaveContext.magicLevel = 0;
            break;
        case ShopItem::Count:
            break;
    }
}

// EnDns calls this after Yes.
void Pay(EnDns* enDns) {
    Slot* slot = FindSlot(&enDns->actor);
    if (slot == nullptr) {
        return;
    }
    if (slot->startsWave) {
        SPDLOG_INFO("[ARENA] Shop: start wave {}", sNextWave);
        if (sStartWave != nullptr) {
            sStartWave();
        }
        return;
    }
    Rupees_ChangeBy(-enDns->dnsItemEntry->itemPrice);
    GiveItem(gPlayState, slot->item);
    slot->bought++;
    SPDLOG_INFO("[ARENA] Shop: bought item {} for {} rupees", static_cast<int>(slot->item),
                enDns->dnsItemEntry->itemPrice);
}

void SpawnScrub(PlayState* play, Slot& slot, int index) {
    Player* player = GET_PLAYER(play);
    Vec3f pos = { slot.place.x, slot.place.y, slot.place.z };
    s16 yaw = Math_Vec3f_Yaw(&pos, &player->actor.world.pos);
    Actor* actor =
        Actor_Spawn(&play->actorCtx, play, ACTOR_EN_DNS, pos.x, pos.y, pos.z, 0, yaw, 0, DNS_TYPE_DEKU_NUTS_5);
    if (actor == NULL) {
        return;
    }
    EnDns* enDns = reinterpret_cast<EnDns*>(actor);
    DnsItemEntry& entry = sEntries[index];
    entry.itemPrice = static_cast<s16>(SlotPrice(slot));
    entry.itemAmount = 1;
    entry.getItemId = GI_NONE;
    entry.purchaseableCheck = CanBuy;
    entry.setRupeesAndFlags = Pay;
    enDns->dnsItemEntry = &entry;
    actor->textId = slot.startsWave ? kStartTextId : kItemTextId;
    if (slot.startsWave) {
        Actor_SetScale(actor, kStartScrubScale);
    }
    slot.scrub = actor;

    Vec3f zero = { 0.0f, 0.0f, 0.0f };
    EffectSsDeadDb_Spawn(play, &pos, &zero, &zero, 100, 20, 255, 255, 255, 200, 120, 120, 255, 1, 9, false);
}

} // namespace

ArenaRules::LinkState CurrentLink() {
    ArenaRules::LinkState link;
    auto own = [&link](ShopItem item, bool value) { link.owns[static_cast<int>(item)] = value; };
    own(ShopItem::BombBag, CUR_UPG_VALUE(UPG_BOMB_BAG) > 0);
    own(ShopItem::MirrorShield, CHECK_OWNED_EQUIP(EQUIP_TYPE_SHIELD, EQUIP_INV_SHIELD_MIRROR) != 0);
    own(ShopItem::MagicDinsFire, gSaveContext.isMagicAcquired);
    own(ShopItem::Longshot, INV_CONTENT(ITEM_HOOKSHOT) == ITEM_LONGSHOT);
    own(ShopItem::MegatonHammer, INV_CONTENT(ITEM_HAMMER) == ITEM_HAMMER);
    own(ShopItem::HoverBoots, CHECK_OWNED_EQUIP(EQUIP_TYPE_BOOTS, EQUIP_INV_BOOTS_HOVER) != 0);
    own(ShopItem::BiggoronSword, CHECK_OWNED_EQUIP(EQUIP_TYPE_SWORD, EQUIP_INV_SWORD_BIGGORON) != 0);
    own(ShopItem::FireArrows, INV_CONTENT(ITEM_ARROW_FIRE) == ITEM_ARROW_FIRE);
    own(ShopItem::IceArrows, INV_CONTENT(ITEM_ARROW_ICE) == ITEM_ARROW_ICE);
    own(ShopItem::LightArrows, INV_CONTENT(ITEM_ARROW_LIGHT) == ITEM_ARROW_LIGHT);
    own(ShopItem::NayrusLove, INV_CONTENT(ITEM_NAYRUS_LOVE) == ITEM_NAYRUS_LOVE);
    own(ShopItem::DoubleDefense, gSaveContext.isDoubleDefenseAcquired);
    own(ShopItem::DoubleMagic, gSaveContext.isDoubleMagicAcquired);
    // A Like-Like can eat the Hylian Shield. The Mirror Shield stays.
    link.hasShield = CHECK_OWNED_EQUIP(EQUIP_TYPE_SHIELD, EQUIP_INV_SHIELD_HYLIAN) != 0 ||
                     CHECK_OWNED_EQUIP(EQUIP_TYPE_SHIELD, EQUIP_INV_SHIELD_MIRROR) != 0;
    link.hearts = gSaveContext.healthCapacity / 16;
    return link;
}

ArenaRules::ShopPlan Plan(int shop) {
    auto random = []() { return Rand_ZeroOne(); };
    return ArenaRules::PlanShop(shop, CurrentLink(), sLastGear, random);
}

void Open(int shop, int nextWave, const ArenaRules::ShopPlan& plan, const Places& places, void (*startWave)()) {
    Close();
    for (int stall = 0; stall < ArenaRules::kStallCount; stall++) {
        if (places.stalls[stall].size() != plan.stalls[stall].size()) {
            SPDLOG_ERROR("[ARENA] Shop: {} places for {} scrubs in stall {}", places.stalls[stall].size(),
                         plan.stalls[stall].size(), stall);
            return;
        }
    }
    sOpen = true;
    sShop = shop;
    sNextWave = nextWave;
    sStartWave = startWave;
    sSlots.push_back(Slot{ true, ShopItem::Count, places.start, nullptr, 0, 0 });
    for (int stall = 0; stall < ArenaRules::kStallCount; stall++) {
        for (size_t i = 0; i < plan.stalls[stall].size(); i++) {
            sSlots.push_back(Slot{ false, plan.stalls[stall][i], places.stalls[stall][i], nullptr, 0, 0 });
        }
    }
    sLastGear = plan.Stall(ArenaRules::ShopStall::Gear);
    SPDLOG_INFO("[ARENA] Shop {} opens with {} scrubs", shop, sSlots.size());
}

void Close() {
    for (Slot& slot : sSlots) {
        if (slot.scrub != nullptr && slot.scrub->update != NULL) {
            Actor_Kill(slot.scrub);
        }
    }
    sSlots.clear();
    sOpen = false;
    sStartWave = nullptr;
}

void Reset() {
    sSlots.clear();
    sOpen = false;
    sStartWave = nullptr;
    sLastGear.clear();
}

bool IsOpen() {
    return sOpen;
}

void Update(PlayState* play) {
    if (!sOpen || play->msgCtx.msgMode != MSGMODE_NONE) {
        return;
    }
    ArenaRules::LinkState link = CurrentLink();
    for (size_t i = 0; i < sSlots.size(); i++) {
        Slot& slot = sSlots[i];
        bool sold = IsSold(slot, link);
        if (slot.scrub != nullptr) {
            // For example, the Hylian Shield after Link buys the Mirror Shield.
            EnDns* enDns = reinterpret_cast<EnDns*>(slot.scrub);
            if (!sold && slot.scrub->update != NULL && enDns->actionFunc == EnDns_Idle) {
                Actor_Kill(slot.scrub);
            }
            continue;
        }
        if (!sold) {
            continue;
        }
        if (slot.respawnFramesLeft > 0) {
            slot.respawnFramesLeft--;
            continue;
        }
        SpawnScrub(play, slot, static_cast<int>(i));
    }
}

void SceneLoaded() {
    for (Slot& slot : sSlots) {
        slot.scrub = nullptr;
        slot.respawnFramesLeft = 0;
    }
}

void Draw(PlayState* play) {
    if (!sOpen) {
        return;
    }
    for (const Slot& slot : sSlots) {
        if (slot.startsWave || slot.scrub == nullptr || slot.scrub->update == NULL) {
            continue;
        }
        if (GoesIntoTheFloor(reinterpret_cast<EnDns*>(slot.scrub))) {
            continue;
        }
        DrawShopItem(play, slot.scrub, slot.item);
    }
}

void OnActorUpdate(void* refActor) {
    Actor* actor = static_cast<Actor*>(refActor);
    Slot* slot = FindSlot(actor);
    if (slot == nullptr) {
        return;
    }
    EnDns* enDns = reinterpret_cast<EnDns*>(actor);
    // EnDns_Update sets textId in each frame. This hook runs after EnDns_Update.
    actor->textId = slot->startsWave ? kStartTextId : kItemTextId;
    // No 3 hearts after a sale (EnDns_PostBurrow).
    enDns->dropCollectible = 0;
}

void OnActorDestroy(void* refActor) {
    Slot* slot = FindSlot(static_cast<Actor*>(refActor));
    if (slot != nullptr) {
        slot->scrub = nullptr;
        slot->respawnFramesLeft = kRespawnFrames;
    }
}

void OnOpenText(uint16_t* textId, bool* loadFromMessageTable) {
    if (!sOpen || (*textId != kItemTextId && *textId != kStartTextId)) {
        return;
    }
    Slot* slot = FindSlot(GET_PLAYER(gPlayState)->talkActor);
    if (slot == nullptr) {
        return;
    }

    // Scrub sound, as in MerchantMessages.cpp.
    const std::string sound = "\x12\x38\x82";
    CustomMessage msg;
    if (slot->startsWave) {
        const std::string wave = std::to_string(sNextWave);
        const std::string yesNo = "%w?\x1B%g";
        msg = CustomMessage(sound + "Start %rwave " + wave + yesNo + "Yes&No%w",
                            sound + "%rWelle " + wave + "%w starten" + yesNo + "Ja&Nein%w",
                            sound + "Lancer la %rvague " + wave + " %w" + yesNo + "Oui&Non%w");
    } else {
        const ItemText& text = kItemTexts[static_cast<int>(slot->item)];
        const std::string price = std::to_string(SlotPrice(*slot));
        // Goes to the Yes/No question.
        const std::string question = "%w\x07\x10\xA3";
        msg = CustomMessage(sound + "%g" + text.english + "%w: %y" + price + " Rupees" + question,
                            sound + "%g" + text.german + "%w: %y" + price + " Rubine" + question,
                            sound + "%g" + text.french + "%w : %y" + price + " rubis" + question);
    }
    msg.AutoFormat();
    msg.LoadIntoFont();
    *loadFromMessageTable = false;
}

void OnGiveItemFromScrub(void* scrub, bool* should) {
    // Pay gives the item.
    if (FindSlot(static_cast<Actor*>(scrub)) != nullptr) {
        *should = false;
    }
}

} // namespace ArenaShop
