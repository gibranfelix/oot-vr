#pragma once

#include <stdint.h>

// Touch controller control of the SoH menu. Pure logic: no OpenXR and no ImGui, so the host unit
// tests can drive it. The window layer feeds it the raw controller state once per frame and turns
// its navigation keys into ImGui gamepad keys. The game's controller reads (VR_GetControllerButton
// and friends) go through FilterGameButtons/GameInputBlocked, so while the menu is open the game
// sees released controls, and a button still held when the menu closes stays released for the game
// until the player lets go of it.
//
// Button masks are the VR_BTN_* bits of vr_interface.h. Hand 0 is left, hand 1 is right.
class VrMenuInput {
  public:
    enum NavKey {
        kNavUp,
        kNavDown,
        kNavLeft,
        kNavRight,
        kNavAccept,  // A
        kNavBack,    // B
        kNavPrevTab, // left grip
        kNavNextTab, // right grip
        kNavKeyCount,
    };

    // One call per frame, before the GUI reads its input. buttons: raw masks per hand. stickX/Y:
    // raw left thumbstick. menuVisible: the menu is visible now. Returns true on a fresh push of the
    // left Y button; the caller then opens or closes the menu, and the new state is assumed from
    // here on.
    bool UpdateFrame(const uint16_t buttons[2], float stickX, float stickY, bool menuVisible);

    // True while the menu is open: the controllers operate the menu, not the game.
    bool GameInputBlocked() const;
    // The buttons of one hand that the game may see. Call with the raw mask of that hand.
    uint16_t FilterGameButtons(int hand, uint16_t raw);

    // State of a navigation key for this frame.
    bool NavKeyDown(NavKey key) const;

    // The game turns the left Y button over to its own bindings while this is false (the ocarina
    // set uses every input). Closing an open menu still works.
    void SetOpenAllowed(bool allowed);
    // Menu code that reads the controllers itself (the VR Inputs binding listener) calls this each
    // frame. For the next frame, navigation and the Y toggle stand down.
    void HoldNavigation();

  private:
    bool mMenuOpen = false;
    bool mOpenAllowed = true;
    bool mHoldRequested = false;
    bool mHeld = false;
    bool mPrevToggleDown = false;
    // Buttons held when the menu closed: hidden from the game until released.
    uint16_t mGameLatched[2] = { 0, 0 };
    // Buttons held when navigation started: no menu action until released.
    uint16_t mNavLatched[2] = { 0, 0 };
    bool mNavKeys[kNavKeyCount] = {};
};

// The one instance the VR layer and the window layer share.
VrMenuInput& vr_menu_input();
