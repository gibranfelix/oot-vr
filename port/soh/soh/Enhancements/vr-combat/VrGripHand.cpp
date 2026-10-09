extern "C" {
#include "z64.h"
#include "macros.h"
#include "functions.h"
#include "variables.h"
extern PlayState* gPlayState;
extern Gfx** sPlayerDListGroups[]; // z_player_lib.c
}

#include "VrCombat.h"

#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/ShipInit.hpp"
#include <libultraship/bridge/consolevariablebridge.h>
#include <vr_interface.h>

// In VR first person, the grip closes an empty hand of Link. The hand holds the bomb with the grip
// (VrBomb.cpp). A hand with an item keeps its model.

namespace {

struct GripHand {
    bool closed = false; // This file closed the hand.
    u8 modelGroup = 0;   // The model group when the hand closed.
};

GripHand sHands[2]; // 0: the L_HAND limb, 1: the R_HAND limb.

// The L_HAND limb follows the sword-hand controller (HeldItemVrHand in VrItemSelect.cpp).
int SwordHand() {
    return CVarGetInteger("gVrLeftHanded", 0) ? VR_HAND_LEFT : VR_HAND_RIGHT;
}

void UpdateHand(Player* player, GripHand* g, u8* type, Gfx*** dLists, u8 open, u8 closed, bool grip) {
    if (g->closed && ((*type != closed) || (player->modelGroup != g->modelGroup))) {
        // The game changed the hand model.
        g->closed = false;
    }
    if (grip && (*type == open)) {
        *type = closed;
        *dLists = &sPlayerDListGroups[closed][gSaveContext.linkAge];
        g->closed = true;
        g->modelGroup = player->modelGroup;
    } else if (!grip && g->closed) {
        *type = open;
        *dLists = &sPlayerDListGroups[open][gSaveContext.linkAge];
        g->closed = false;
    }
}

void OnPlayerUpdateGripHand() {
    if (gPlayState == NULL) {
        return;
    }
    Player* player = GET_PLAYER(gPlayState);
    if (player == NULL) {
        return;
    }
    const bool on = VR_IsInitialized() && VR_GetFirstPerson() && CVarGetInteger("gVrMotionHands", 1) &&
                    VrCombat_InPlay();
    const int swordHand = SwordHand();
    const bool swordGrip = on && (VR_GetControllerButton(swordHand) & VR_BTN_GRIP);
    const bool offGrip = on && (VR_GetControllerButton(swordHand ^ 1) & VR_BTN_GRIP);
    UpdateHand(player, &sHands[0], &player->leftHandType, &player->leftHandDLists, PLAYER_MODELTYPE_LH_OPEN,
               PLAYER_MODELTYPE_LH_CLOSED, swordGrip);
    UpdateHand(player, &sHands[1], &player->rightHandType, &player->rightHandDLists, PLAYER_MODELTYPE_RH_OPEN,
               PLAYER_MODELTYPE_RH_CLOSED, offGrip);
}

} // namespace

static void RegisterVrGripHand() {
    COND_HOOK(OnPlayerUpdate, true, OnPlayerUpdateGripHand);
}

static RegisterShipInitFunc initVrGripHand(RegisterVrGripHand, {});
