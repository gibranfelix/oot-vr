#pragma once

// ARENA shop (#169). Arena.cpp calls these functions only in ARENA. The rules are in ArenaRules.h.

#include "ArenaRules.h"

#include <cstdint>
#include <vector>

struct PlayState;

namespace ArenaShop {

// From gSaveContext.
ArenaRules::LinkState CurrentLink();

// Gear at random: see ArenaRules::PlanShop.
ArenaRules::ShopPlan Plan(int shop);

// Scene coordinates.
struct Place {
    float x;
    float y;
    float z;
};

// One place for the wave scrub, and one for each item of the plan.
struct Places {
    Place start;
    std::vector<Place> stalls[ArenaRules::kStallCount];
};

// startWave runs when the player says Yes to the wave scrub.
void Open(int shop, int nextWave, const ArenaRules::ShopPlan& plan, const Places& places, void (*startWave)());

// Kills all the scrubs.
void Close();

// For a new run. Does not kill the scrubs: the scene can be gone.
void Reset();

bool IsOpen();

// Call each frame. Spawns the scrubs of the items on sale, and kills the others.
void Update(PlayState* play);

// The scene loaded again and the scrubs are gone.
void SceneLoaded();

// Draws the item above each scrub. Call after the actors draw.
void Draw(PlayState* play);

void OnActorUpdate(void* actor);
void OnActorDestroy(void* actor);
void OnOpenText(uint16_t* textId, bool* loadFromMessageTable);
// For VB_GIVE_ITEM_FROM_BUSINESS_SCRUB.
void OnGiveItemFromScrub(void* scrub, bool* should);

} // namespace ArenaShop
