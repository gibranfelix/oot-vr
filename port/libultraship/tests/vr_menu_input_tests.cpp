#include <gtest/gtest.h>
#include "fast/vr_menu_input.h"

namespace {

// VR_BTN_* bits of vr_interface.h.
constexpr uint16_t kTrigger = 1 << 0;
constexpr uint16_t kGrip = 1 << 1;
constexpr uint16_t kPrimary = 1 << 2;   // A (right) / X (left)
constexpr uint16_t kSecondary = 1 << 3; // B (right) / Y (left)

constexpr int kLeft = 0;
constexpr int kRight = 1;

// Drives one VrMenuInput the way the window layer does: the caller honors every toggle.
struct Harness {
    VrMenuInput input;
    bool menuVisible = false;

    bool Frame(uint16_t left, uint16_t right, float stickX = 0.0f, float stickY = 0.0f) {
        const uint16_t buttons[2] = { left, right };
        const bool toggle = input.UpdateFrame(buttons, stickX, stickY, menuVisible);
        if (toggle) {
            menuVisible = !menuVisible;
        }
        return toggle;
    }

    void OpenMenu() {
        Frame(kSecondary, 0);
        Frame(0, 0);
    }
};

} // namespace

TEST(VrMenuInputTest, LeftYPushOpensAndSecondPushCloses) {
    Harness h;
    EXPECT_TRUE(h.Frame(kSecondary, 0));
    EXPECT_TRUE(h.menuVisible);
    EXPECT_FALSE(h.Frame(kSecondary, 0)); // still held: no second toggle
    EXPECT_FALSE(h.Frame(0, 0));
    EXPECT_TRUE(h.Frame(kSecondary, 0));
    EXPECT_FALSE(h.menuVisible);
}

TEST(VrMenuInputTest, RightBDoesNotToggle) {
    Harness h;
    EXPECT_FALSE(h.Frame(0, kSecondary));
    EXPECT_FALSE(h.menuVisible);
}

TEST(VrMenuInputTest, GameNeverSeesLeftYWhileTheMenuButtonIsActive) {
    Harness h;
    EXPECT_EQ(h.input.FilterGameButtons(kLeft, kSecondary | kTrigger), kTrigger);
    EXPECT_EQ(h.input.FilterGameButtons(kRight, kSecondary), kSecondary);
}

TEST(VrMenuInputTest, OcarinaKeepsLeftYAndCannotOpenTheMenu) {
    Harness h;
    h.input.SetOpenAllowed(false);
    EXPECT_EQ(h.input.FilterGameButtons(kLeft, kSecondary), kSecondary);
    EXPECT_FALSE(h.Frame(kSecondary, 0));
    EXPECT_FALSE(h.menuVisible);
}

TEST(VrMenuInputTest, OpenMenuClosesEvenWhenOpeningIsNotAllowed) {
    Harness h;
    h.OpenMenu();
    h.input.SetOpenAllowed(false);
    EXPECT_TRUE(h.Frame(kSecondary, 0));
    EXPECT_FALSE(h.menuVisible);
}

TEST(VrMenuInputTest, YHeldWhenOpeningBecomesAllowedDoesNotToggle) {
    Harness h;
    h.input.SetOpenAllowed(false);
    h.Frame(kSecondary, 0);
    h.input.SetOpenAllowed(true);
    EXPECT_FALSE(h.Frame(kSecondary, 0));
}

TEST(VrMenuInputTest, GameSeesNoButtonsWhileTheMenuIsOpen) {
    Harness h;
    h.OpenMenu();
    EXPECT_TRUE(h.input.GameInputBlocked());
    EXPECT_EQ(h.input.FilterGameButtons(kLeft, kTrigger | kGrip), 0);
    EXPECT_EQ(h.input.FilterGameButtons(kRight, kPrimary | kSecondary), 0);
}

TEST(VrMenuInputTest, GameIsNotBlockedWithTheMenuClosed) {
    Harness h;
    h.Frame(0, 0);
    EXPECT_FALSE(h.input.GameInputBlocked());
    EXPECT_EQ(h.input.FilterGameButtons(kRight, kPrimary | kTrigger), kPrimary | kTrigger);
}

TEST(VrMenuInputTest, ButtonHeldAtCloseStaysReleasedForTheGameUntilLetGo) {
    Harness h;
    h.OpenMenu();
    // A activates the close button: the menu closes while A is down.
    h.Frame(0, kPrimary);
    h.menuVisible = false;
    h.Frame(0, kPrimary);
    EXPECT_FALSE(h.input.GameInputBlocked());
    EXPECT_EQ(h.input.FilterGameButtons(kRight, kPrimary), 0);
    EXPECT_EQ(h.input.FilterGameButtons(kRight, 0), 0);
    EXPECT_EQ(h.input.FilterGameButtons(kRight, kPrimary), kPrimary); // a new push reaches the game
}

TEST(VrMenuInputTest, ClosingYIsNotSeenByTheGame) {
    Harness h;
    h.OpenMenu();
    h.Frame(kSecondary, 0); // closes
    h.input.SetOpenAllowed(false); // even if the ocarina comes up right after
    EXPECT_EQ(h.input.FilterGameButtons(kLeft, kSecondary), 0);
}

TEST(VrMenuInputTest, NavigationKeysFollowTheControllersWhileOpen) {
    Harness h;
    h.OpenMenu();
    h.Frame(kGrip, kPrimary | kSecondary, 0.0f, 0.9f);
    EXPECT_TRUE(h.input.NavKeyDown(VrMenuInput::kNavUp));
    EXPECT_FALSE(h.input.NavKeyDown(VrMenuInput::kNavDown));
    EXPECT_TRUE(h.input.NavKeyDown(VrMenuInput::kNavAccept));
    EXPECT_TRUE(h.input.NavKeyDown(VrMenuInput::kNavBack));
    EXPECT_TRUE(h.input.NavKeyDown(VrMenuInput::kNavPrevTab));
    EXPECT_FALSE(h.input.NavKeyDown(VrMenuInput::kNavNextTab));

    h.Frame(0, kGrip, -0.9f, 0.2f);
    EXPECT_TRUE(h.input.NavKeyDown(VrMenuInput::kNavLeft));
    EXPECT_FALSE(h.input.NavKeyDown(VrMenuInput::kNavUp));
    EXPECT_TRUE(h.input.NavKeyDown(VrMenuInput::kNavNextTab));
    EXPECT_FALSE(h.input.NavKeyDown(VrMenuInput::kNavAccept));

    h.Frame(0, 0, 0.3f, -0.3f); // inside the dead zone
    for (int k = 0; k < VrMenuInput::kNavKeyCount; k++) {
        EXPECT_FALSE(h.input.NavKeyDown(static_cast<VrMenuInput::NavKey>(k)));
    }
}

TEST(VrMenuInputTest, NoNavigationWhileTheMenuIsClosed) {
    Harness h;
    h.Frame(0, kPrimary, 0.9f, 0.0f);
    EXPECT_FALSE(h.input.NavKeyDown(VrMenuInput::kNavAccept));
    EXPECT_FALSE(h.input.NavKeyDown(VrMenuInput::kNavRight));
}

TEST(VrMenuInputTest, ButtonHeldWhenTheMenuOpensDoesNothingUntilReleased) {
    Harness h;
    h.Frame(kSecondary | kGrip, 0); // the shield grip is up when the player opens the menu
    h.Frame(kGrip, 0);
    EXPECT_FALSE(h.input.NavKeyDown(VrMenuInput::kNavPrevTab));
    h.Frame(0, 0);
    h.Frame(kGrip, 0);
    EXPECT_TRUE(h.input.NavKeyDown(VrMenuInput::kNavPrevTab));
}

TEST(VrMenuInputTest, ClosingReleasesAllNavigationKeys) {
    Harness h;
    h.OpenMenu();
    h.Frame(kSecondary, kPrimary, 0.0f, 0.9f); // closes
    for (int k = 0; k < VrMenuInput::kNavKeyCount; k++) {
        EXPECT_FALSE(h.input.NavKeyDown(static_cast<VrMenuInput::NavKey>(k)));
    }
}

TEST(VrMenuInputTest, HoldNavigationSuspendsNavigationAndTheToggleForOneFrame) {
    Harness h;
    h.OpenMenu();
    h.input.HoldNavigation();
    EXPECT_FALSE(h.Frame(kSecondary, kPrimary));
    EXPECT_TRUE(h.menuVisible);
    EXPECT_FALSE(h.input.NavKeyDown(VrMenuInput::kNavAccept));
    EXPECT_TRUE(h.input.GameInputBlocked());
}

TEST(VrMenuInputTest, ButtonsHeldWhenTheHoldEndsDoNothingUntilReleased) {
    Harness h;
    h.OpenMenu();
    h.input.HoldNavigation();
    h.Frame(0, kPrimary); // the listener binds A
    h.Frame(0, kPrimary); // the hold has ended, A is still down
    EXPECT_FALSE(h.input.NavKeyDown(VrMenuInput::kNavAccept));
    h.Frame(0, 0);
    h.Frame(0, kPrimary);
    EXPECT_TRUE(h.input.NavKeyDown(VrMenuInput::kNavAccept));
}

TEST(VrMenuInputTest, YHeldAcrossTheHoldDoesNotToggleAfterIt) {
    Harness h;
    h.OpenMenu();
    h.input.HoldNavigation();
    h.Frame(kSecondary, 0);
    EXPECT_FALSE(h.Frame(kSecondary, 0));
    EXPECT_TRUE(h.menuVisible);
}

TEST(VrMenuInputTest, StickPushedAtCloseStaysCenteredForTheGameUntilReleased) {
    Harness h;
    h.OpenMenu();
    h.Frame(kSecondary, 0, 0.0f, 0.9f); // closes with the left stick pushed
    float x = 0.0f;
    float y = 0.9f;
    h.input.FilterGameStick(kLeft, &x, &y);
    EXPECT_EQ(x, 0.0f);
    EXPECT_EQ(y, 0.0f);
    x = 0.05f;
    y = 0.0f; // back to the center
    h.input.FilterGameStick(kLeft, &x, &y);
    x = 0.0f;
    y = 0.9f;
    h.input.FilterGameStick(kLeft, &x, &y);
    EXPECT_EQ(y, 0.9f);
}

TEST(VrMenuInputTest, GameSticksReadCenteredWhileTheMenuIsOpen) {
    Harness h;
    h.OpenMenu();
    float x = 0.7f;
    float y = -0.7f;
    h.input.FilterGameStick(kRight, &x, &y);
    EXPECT_EQ(x, 0.0f);
    EXPECT_EQ(y, 0.0f);
}

TEST(VrMenuInputTest, StickIsUntouchedWithTheMenuClosed) {
    Harness h;
    h.Frame(0, 0);
    float x = 0.7f;
    float y = -0.7f;
    h.input.FilterGameStick(kRight, &x, &y);
    EXPECT_EQ(x, 0.7f);
    EXPECT_EQ(y, -0.7f);
}

TEST(VrMenuInputTest, HiddenButtonsCoverTheAnalogTriggerAndGrip) {
    Harness h;
    h.OpenMenu();
    EXPECT_EQ(h.input.GameHiddenButtons(kRight), 0xFFFF);
    h.Frame(kSecondary, kTrigger); // closes with the right trigger down
    h.input.FilterGameButtons(kRight, kTrigger);
    EXPECT_TRUE(h.input.GameHiddenButtons(kRight) & kTrigger);
    h.input.FilterGameButtons(kRight, 0);
    EXPECT_FALSE(h.input.GameHiddenButtons(kRight) & kTrigger);
}
