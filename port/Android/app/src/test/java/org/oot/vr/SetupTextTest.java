package org.oot.vr;

import static org.junit.Assert.assertEquals;

import org.junit.Test;

public class SetupTextTest {

    @Test
    public void progressShowsDoneOfTotal() {
        assertEquals("Extracting the game assets… 12 / 547", SetupText.extracting(12, 547));
    }

    @Test
    public void progressBeforeTheTotalIsKnownShowsNoNumbers() {
        assertEquals("Extracting the game assets…", SetupText.extracting(0, 0));
    }

    @Test
    public void progressNeverShowsMoreDoneThanTotal() {
        assertEquals("Extracting the game assets… 547 / 547", SetupText.extracting(600, 547));
    }

    @Test
    public void negativeDoneShowsAsZero() {
        assertEquals("Extracting the game assets… 0 / 547", SetupText.extracting(-1, 547));
    }

    @Test
    public void barPositionScalesToTheBarMaximum() {
        assertEquals(0, SetupText.barPosition(0, 0));
        assertEquals(0, SetupText.barPosition(0, 547));
        assertEquals(SetupText.BAR_MAX / 2, SetupText.barPosition(50, 100));
        assertEquals(SetupText.BAR_MAX, SetupText.barPosition(547, 547));
        assertEquals(SetupText.BAR_MAX, SetupText.barPosition(600, 547));
    }

    @Test
    public void introTextIsTheAgreedText() {
        assertEquals("You must own a legal copy of The Legend of Zelda: Ocarina of Time. "
            + "Select the dump of your own cartridge or disc (.z64, .n64, or .v64).",
            SetupText.INTRO);
    }

    @Test
    public void missingPickerTellsThePlayerWhatToDoInstead() {
        assertEquals("The headset has no file picker. Make oot.o2r with Ship of Harkinian for PC. "
            + "Read the README of this project.", SetupText.NO_PICKER);
    }

    @Test
    public void secondSetupInOneProcessAsksForARestart() {
        assertEquals("Close the app and open it again to select the ROM.", SetupText.RESTART_NEEDED);
    }
}
