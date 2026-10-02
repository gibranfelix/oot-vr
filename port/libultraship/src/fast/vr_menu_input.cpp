#include "fast/vr_menu_input.h"

namespace {
// VR_BTN_* bits of vr_interface.h (this file stays free of the VR headers for the host tests).
constexpr uint16_t kBtnGrip = 1 << 1;
constexpr uint16_t kBtnPrimary = 1 << 2;
constexpr uint16_t kBtnSecondary = 1 << 3;

constexpr int kLeft = 0;
constexpr int kRight = 1;

// The menu button: left Y. The right Menu button belongs to Horizon OS.
constexpr int kToggleHand = kLeft;
constexpr uint16_t kToggleMask = kBtnSecondary;

// Same dominant-axis rule and threshold as the stick C-buttons in padmgr.c.
constexpr float kStickThreshold = 0.5f;
} // namespace

bool VrMenuInput::UpdateFrame(const uint16_t buttons[2], float stickX, float stickY, bool menuVisible) {
    const bool wasHeld = mHeld;
    mHeld = mHoldRequested;
    mHoldRequested = false;

    const bool toggleDown = (buttons[kToggleHand] & kToggleMask) != 0;
    const bool toggle =
        toggleDown && !mPrevToggleDown && !mHeld && (menuVisible || mOpenAllowed);
    mPrevToggleDown = toggleDown;

    if (menuVisible) {
        // Everything held now stays hidden from the game after the menu closes.
        mGameLatched[kLeft] = buttons[kLeft];
        mGameLatched[kRight] = buttons[kRight];
    } else {
        mGameLatched[kLeft] &= buttons[kLeft];
        mGameLatched[kRight] &= buttons[kRight];
    }

    const bool wasNavigating = mMenuOpen && !wasHeld;
    mMenuOpen = menuVisible != toggle;
    const bool navigating = mMenuOpen && !mHeld;

    for (bool& key : mNavKeys) {
        key = false;
    }
    if (!navigating) {
        return toggle;
    }

    // A button already down when navigation starts (the shield grip when the menu opens, the
    // input that the binding listener just took) must not act on the menu: wait for its release.
    for (int hand = kLeft; hand <= kRight; hand++) {
        mNavLatched[hand] = wasNavigating ? (mNavLatched[hand] & buttons[hand]) : buttons[hand];
    }
    const uint16_t left = buttons[kLeft] & ~mNavLatched[kLeft];
    const uint16_t right = buttons[kRight] & ~mNavLatched[kRight];

    if ((stickY * stickY) >= (stickX * stickX)) {
        mNavKeys[kNavUp] = stickY > kStickThreshold;
        mNavKeys[kNavDown] = stickY < -kStickThreshold;
    } else {
        mNavKeys[kNavLeft] = stickX < -kStickThreshold;
        mNavKeys[kNavRight] = stickX > kStickThreshold;
    }
    mNavKeys[kNavAccept] = (right & kBtnPrimary) != 0;
    mNavKeys[kNavBack] = (right & kBtnSecondary) != 0;
    mNavKeys[kNavPrevTab] = (left & kBtnGrip) != 0;
    mNavKeys[kNavNextTab] = (right & kBtnGrip) != 0;
    return toggle;
}

bool VrMenuInput::GameInputBlocked() const {
    return mMenuOpen;
}

uint16_t VrMenuInput::FilterGameButtons(int hand, uint16_t raw) {
    if (hand < kLeft || hand > kRight) {
        return raw;
    }
    if (mMenuOpen) {
        mGameLatched[hand] = raw;
        return 0;
    }
    mGameLatched[hand] &= raw;
    uint16_t hidden = mGameLatched[hand];
    if (hand == kToggleHand && mOpenAllowed) {
        hidden |= kToggleMask;
    }
    return raw & ~hidden;
}

bool VrMenuInput::NavKeyDown(NavKey key) const {
    return key >= 0 && key < kNavKeyCount && mNavKeys[key];
}

void VrMenuInput::SetOpenAllowed(bool allowed) {
    mOpenAllowed = allowed;
}

void VrMenuInput::HoldNavigation() {
    mHoldRequested = true;
}

VrMenuInput& vr_menu_input() {
    static VrMenuInput sInstance;
    return sInstance;
}
