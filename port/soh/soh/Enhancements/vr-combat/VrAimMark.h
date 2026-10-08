#pragma once

// The color of the aim mark (the hookshot reticle). No game types: the host tests in
// port/soh/tests/vr/VrAimMarkTests.cpp build it alone.

namespace VrAimMark {

enum class Mark {
    Vanilla,    // The game draws its own reticle.
    Holds,      // Green: the hook can hold.
    CannotHold, // Red: the hook cannot hold.
};

// holdColors: Enhancements > Items > Targetable Hookshot Reticle.
// customCannotHoldColor: the player changed the red color in the cosmetics editor.
inline Mark MarkFor(bool vrOn, bool holdColors, bool customCannotHoldColor, bool hookshotHeld, bool surfaceHolds) {
    const bool showHoldColors = vrOn || holdColors;
    if (!showHoldColors && !customCannotHoldColor) {
        return Mark::Vanilla;
    }
    return (showHoldColors && hookshotHeld && surfaceHolds) ? Mark::Holds : Mark::CannotHold;
}

} // namespace VrAimMark
