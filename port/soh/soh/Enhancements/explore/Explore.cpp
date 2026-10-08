#include "Explore.h"
#include "soh/SaveManager.h"

#include <cstring>

extern "C" {
#include <z64.h>
#include "macros.h"
#include "variables.h"
#include "functions.h"

extern SaveContext gSaveContext;
}

extern "C" void Explore_InitSave(void) {
    // InitFile resets the fields that file select already set.
    u8 playerName[ARRAY_COUNT(gSaveContext.playerName)];
    memcpy(playerName, gSaveContext.playerName, sizeof(playerName));
    u8 filenameLanguage = gSaveContext.ship.filenameLanguage;
    s16 n64ddFlag = gSaveContext.n64ddFlag;

    // InitFile also sets the quest: QUEST_NORMAL, or QUEST_MASTER when only Master Quest is available.
    SaveManager::Instance->InitFile(false);
    // Set the age first. InitFileMaxed gives a child the Kokiri Sword and the slingshot.
    gSaveContext.linkAge = LINK_AGE_ADULT;
    SaveManager::InitFileMaxed();

    memcpy(gSaveContext.playerName, playerName, sizeof(playerName));
    gSaveContext.ship.filenameLanguage = filenameLanguage;
    gSaveContext.n64ddFlag = n64ddFlag;

    // Start in the Temple of Time at noon, without the intro cutscene. Sram_OpenSave starts an adult at the warp pad
    // of the Temple of Time when the saved scene is not Link's house.
    gSaveContext.entranceIndex = ENTR_TEMPLE_OF_TIME_WARP_PAD;
    gSaveContext.savedSceneNum = SCENE_TEMPLE_OF_TIME;
    gSaveContext.dayTime = gSaveContext.skyboxTime = CLOCK_TIME(12, 0);
    gSaveContext.cutsceneIndex = 0;

    // Skip the cutscenes of Rauru and Sheik at the Master Sword pedestal.
    Flags_SetEventChkInf(EVENTCHKINF_PULLED_MASTER_SWORD_FROM_PEDESTAL);
    Flags_SetEventChkInf(EVENTCHKINF_SHEIK_SPAWNED_AT_MASTER_SWORD_PEDESTAL);

    // C buttons: the hookshot, the bow, and the ocarina.
    static const u8 sAdultItems[] = { ITEM_LONGSHOT, ITEM_BOW, ITEM_OCARINA_TIME };
    static const u8 sAdultSlots[] = { SLOT_HOOKSHOT, SLOT_BOW, SLOT_OCARINA };
    for (int i = 0; i < ARRAY_COUNT(sAdultItems); i++) {
        gSaveContext.equips.buttonItems[i + 1] = sAdultItems[i];
        gSaveContext.equips.cButtonSlots[i] = sAdultSlots[i];
    }

    // The equipment of the child after the Master Sword pedestal. Without it, the child keeps the adult equipment.
    static const u8 sChildItems[] = { ITEM_SWORD_KOKIRI, ITEM_SLINGSHOT, ITEM_BOOMERANG, ITEM_OCARINA_TIME };
    static const u8 sChildSlots[] = { SLOT_SLINGSHOT, SLOT_BOOMERANG, SLOT_OCARINA };
    for (int i = 0; i < ARRAY_COUNT(sChildItems); i++) {
        gSaveContext.childEquips.buttonItems[i] = sChildItems[i];
    }
    for (int i = 0; i < ARRAY_COUNT(sChildSlots); i++) {
        gSaveContext.childEquips.cButtonSlots[i] = sChildSlots[i];
    }
    gSaveContext.childEquips.equipment =
        (EQUIP_VALUE_SWORD_KOKIRI << (EQUIP_TYPE_SWORD * 4)) | (EQUIP_VALUE_SHIELD_DEKU << (EQUIP_TYPE_SHIELD * 4)) |
        (EQUIP_VALUE_TUNIC_KOKIRI << (EQUIP_TYPE_TUNIC * 4)) | (EQUIP_VALUE_BOOTS_KOKIRI << (EQUIP_TYPE_BOOTS * 4));
}
