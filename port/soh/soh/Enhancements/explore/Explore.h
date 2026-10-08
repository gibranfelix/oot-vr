#pragma once

// The EXPLORE option in the quest selector of file select. An EXPLORE file starts with the maxed save of SoH, so a
// new player can try the port without the intro and the first hours of the story.

#ifdef __cplusplus
extern "C" {
#endif

// Replaces the new save in gSaveContext with the EXPLORE save. Keeps the player name and the name language.
void Explore_InitSave(void);

#ifdef __cplusplus
}
#endif
