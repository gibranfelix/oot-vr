#include "ArenaShop.h"

#include "soh/Enhancements/custom-message/CustomMessageManager.h"

#include <array>
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

// The idle action of the scrub (z_en_dns.c). The shop removes a scrub only when it is idle, so that a scrub that sold
// its item can go into the floor.
void EnDns_Idle(EnDns* enDns, PlayState* play);
}

using ArenaRules::ShopItem;

namespace ArenaShop {
namespace {

// Text IDs that the game does not use. OnOpenText makes the text.
constexpr u16 kItemTextId = 0x9400;
constexpr u16 kStartTextId = 0x9401;
// Text of the scrubs (z_en_dns.c): the Yes/No question after the offer.
constexpr u16 kBuyQuestionTextId = 0x10A3;

// A scrub that went into the floor comes back after 1.5 seconds, when the shop still sells its item.
constexpr int kRespawnFrames = 30;

struct ItemText {
    ShopItem item;
    const char* english;
    const char* german;
    const char* french;
};

constexpr ItemText kItemTexts[ArenaRules::kShopItemCount] = {
    { ShopItem::RedPotion, "a Red Potion", "ein Rotes Elixier", "une Potion rouge" },
    { ShopItem::GreenPotion, "a Green Potion", "ein Grünes Elixier", "une Potion verte" },
    { ShopItem::RecoveryHearts, "3 Recovery Hearts", "3 Herzen", "3 cœurs" },
    { ShopItem::Arrows, "10 Arrows", "10 Pfeile", "10 flèches" },
    { ShopItem::Bombs, "10 Bombs", "10 Bomben", "10 bombes" },
    { ShopItem::HylianShield, "a Hylian Shield", "einen Hylia-Schild", "un Bouclier hylien" },
    { ShopItem::HeartContainer, "a Heart Container", "einen Herzcontainer", "un Réceptacle de cœur" },
    { ShopItem::BombBag, "a Bomb Bag", "eine Bombentasche", "un Sac de bombes" },
    { ShopItem::MirrorShield, "a Mirror Shield", "einen Spiegelschild", "un Bouclier miroir" },
    { ShopItem::DekuNuts, "10 Deku Nuts", "10 Deku-Nüsse", "10 noix Mojo" },
    { ShopItem::MagicDinsFire, "Magic and Din's Fire", "Magie und Dins Feuerinferno", "la Magie et le Feu de Din" },
    { ShopItem::Longshot, "a Longshot", "einen Enterhaken", "un Super grappin" },
    { ShopItem::MegatonHammer, "a Megaton Hammer", "einen Stahlhammer", "une Masse des Titans" },
    { ShopItem::HoverBoots, "Hover Boots", "Gleitstiefel", "des Bottes des airs" },
    { ShopItem::BiggoronSword, "Biggoron's Sword", "das Biggoron-Schwert", "l'Épée de Biggoron" },
    { ShopItem::FireArrows, "Fire Arrows", "Feuerpfeile", "des Flèches de feu" },
    { ShopItem::IceArrows, "Ice Arrows", "Eispfeile", "des Flèches de glace" },
    { ShopItem::LightArrows, "Light Arrows", "Lichtpfeile", "des Flèches de lumière" },
    { ShopItem::NayrusLove, "Nayru's Love", "Nayrus Umarmung", "l'Amour de Nayru" },
    { ShopItem::DoubleDefense, "Double Defense", "doppelte Verteidigung", "la Double défense" },
    { ShopItem::DoubleMagic, "Double Magic", "doppelte Magie", "la Double magie" },
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
    // The scrub that starts the next wave has no item.
    bool startsWave;
    ShopItem item;
    Place place;
    Actor* scrub;
    int respawnFramesLeft;
};

bool sOpen = false;
int sShop = 0;
int sNextWave = 0;
void (*sStartWave)() = nullptr;
std::vector<Slot> sSlots;
// The item of each scrub. EnDns keeps a pointer to its entry, so the entries stay in this array.
std::array<DnsItemEntry, ArenaRules::kShopItemCount + 1> sEntries;

Slot* FindSlot(const Actor* actor) {
    for (Slot& slot : sSlots) {
        if (slot.scrub == actor && actor != nullptr) {
            return &slot;
        }
    }
    return nullptr;
}

bool IsSold(const Slot& slot, const ArenaRules::LinkState& link) {
    return slot.startsWave || ArenaRules::IsOffered(slot.item, sShop, link);
}

u32 CanBuy(EnDns* enDns) {
    Slot* slot = FindSlot(&enDns->actor);
    if (slot == nullptr) {
        return DNS_CANBUY_RESULT_CANT_GET_NOW;
    }
    if (slot->startsWave) {
        return DNS_CANBUY_RESULT_SUCCESS;
    }
    if (!ArenaRules::IsOffered(slot->item, sShop, CurrentLink())) {
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

// Gives the item without the get item animation: the scrub goes into the floor, and the shop continues.
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

// EnDns calls this after the player says Yes and the scrub can sell.
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
    entry.itemPrice = slot.startsWave ? 0 : static_cast<s16>(ArenaRules::Price(slot.item, sShop));
    entry.itemAmount = 1;
    entry.getItemId = GI_NONE;
    entry.purchaseableCheck = CanBuy;
    entry.setRupeesAndFlags = Pay;
    enDns->dnsItemEntry = &entry;
    actor->textId = slot.startsWave ? kStartTextId : kItemTextId;
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

int ScrubCount(int shop) {
    return static_cast<int>(ArenaRules::ShopSlots(shop, CurrentLink()).size()) + 1;
}

void Open(int shop, int nextWave, const std::vector<Place>& places, void (*startWave)()) {
    Close();
    std::vector<ShopItem> items = ArenaRules::ShopSlots(shop, CurrentLink());
    if (places.size() != items.size() + 1) {
        SPDLOG_ERROR("[ARENA] Shop: {} places for {} scrubs", places.size(), items.size() + 1);
        return;
    }
    sOpen = true;
    sShop = shop;
    sNextWave = nextWave;
    sStartWave = startWave;
    // The scrub that starts the wave is the first scrub of the ring.
    sSlots.push_back(Slot{ true, ShopItem::Count, places[0], nullptr, 0 });
    for (size_t i = 0; i < items.size(); i++) {
        sSlots.push_back(Slot{ false, items[i], places[i + 1], nullptr, 0 });
    }
    SPDLOG_INFO("[ARENA] Shop {} opens with {} scrubs", shop, sSlots.size());
}

void Close() {
    for (Slot& slot : sSlots) {
        if (slot.scrub != nullptr && slot.scrub->update != NULL) {
            Actor_Kill(slot.scrub);
        }
    }
    Reset();
}

void Reset() {
    sSlots.clear();
    sOpen = false;
    sStartWave = nullptr;
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
            // An idle scrub of an item that the shop does not sell now goes away, for example the Hylian Shield after
            // Link buys the Mirror Shield.
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

void OnActorUpdate(void* refActor) {
    Actor* actor = static_cast<Actor*>(refActor);
    Slot* slot = FindSlot(actor);
    if (slot == nullptr) {
        return;
    }
    EnDns* enDns = reinterpret_cast<EnDns*>(actor);
    // EnDns_Update sets the text of its params in each frame. This hook runs after EnDns_Update (Actor_UpdateAll), and
    // the player reads textId in the next frame, so the player gets the text of the shop.
    actor->textId = slot->startsWave ? kStartTextId : kItemTextId;
    // A scrub that sold its item drops 3 hearts when it goes into the floor (EnDns_PostBurrow). The shop sells hearts.
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

    // \x12\x38\x82: the sound of the scrub, as in the texts of the randomizer for scrubs (MerchantMessages.cpp).
    const std::string sound = "\x12\x38\x82";
    CustomMessage msg;
    if (slot->startsWave) {
        const std::string wave = std::to_string(sNextWave);
        const std::string yesNo = "%w?\x1B%g";
        msg = CustomMessage(sound + "Are you ready for %rwave " + wave + yesNo + "Yes&No%w",
                            sound + "Bist Du bereit für %rWelle " + wave + yesNo + "Ja&Nein%w",
                            sound + "Es-tu prêt pour la %rvague " + wave + yesNo + "Oui&Non%w");
    } else {
        const ItemText& text = kItemTexts[static_cast<int>(slot->item)];
        const std::string price = std::to_string(ArenaRules::Price(slot->item, sShop));
        // \x07\x10\xA3 goes to the Yes/No question of the scrubs.
        const std::string question = "%w!\x07\x10\xA3";
        msg = CustomMessage(sound + "I sell %g" + text.english + "%w for %y" + price + " Rupees" + question,
                            sound + "Ich verkaufe Dir %g" + text.german + "%w für %y" + price + " Rubine" + question,
                            sound + "Je te vends %g" + text.french + "%w pour %y" + price + " Rubis" + question);
    }
    msg.AutoFormat();
    msg.LoadIntoFont();
    *loadFromMessageTable = false;
}

void OnGiveItemFromScrub(void* scrub, bool* should) {
    // Pay gives the item. The get item animation would stop the shop for each item.
    if (FindSlot(static_cast<Actor*>(scrub)) != nullptr) {
        *should = false;
    }
}

} // namespace ArenaShop
