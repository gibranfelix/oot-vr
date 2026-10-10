package org.oot.vr;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public class GameProcessTest {

    @Test
    public void endedGameEndsTheProcess() {
        assertTrue(GameProcess.endsWithActivity(false, false));
    }

    @Test
    public void handOverToTheSetupKeepsTheProcess() {
        // The setup panel runs in this process and starts MainActivity again from it.
        assertFalse(GameProcess.endsWithActivity(true, false));
    }

    @Test
    public void configurationChangeKeepsTheProcess() {
        // Android creates the activity again at once, in this process.
        assertFalse(GameProcess.endsWithActivity(false, true));
    }
}
