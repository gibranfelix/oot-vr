#pragma once

// The shop of ARENA (issue #169): Business Scrubs (EnDns) sell items between waves. Arena.cpp opens and closes the
// shop, and calls the hooks below only in ARENA. The rules of the catalog are in ArenaRules.h.

#include "ArenaRules.h"

#include <cstdint>
#include <vector>

struct PlayState;

namespace ArenaShop {

// What Link has now, from gSaveContext.
ArenaRules::LinkState CurrentLink();

// The number of scrubs for the shop: one for each slot (ArenaRules::ShopSlots) and one that starts the next wave.
int ScrubCount(int shop);

// A place of a scrub on the floor, in the coordinates of the scene.
struct Place {
    float x;
    float y;
    float z;
};

// Opens the shop. places has ScrubCount(shop) places. nextWave is the wave after the shop. The shop calls startWave
// when the player tells a scrub to start the next wave.
void Open(int shop, int nextWave, const std::vector<Place>& places, void (*startWave)());

// Removes all the scrubs.
void Close();

// Forgets the scrubs, and does not touch them. For a new run: the scrubs of the last run can be deleted.
void Reset();

bool IsOpen();

// Spawns the scrubs of the items that the shop sells now, and removes the others. Call one time in each frame.
void Update(PlayState* play);

// The scene loaded again: the scrubs are gone. Update spawns them again.
void SceneLoaded();

void OnActorUpdate(void* actor);
void OnActorDestroy(void* actor);
void OnOpenText(uint16_t* textId, bool* loadFromMessageTable);
// For VB_GIVE_ITEM_FROM_BUSINESS_SCRUB.
void OnGiveItemFromScrub(void* scrub, bool* should);

} // namespace ArenaShop
