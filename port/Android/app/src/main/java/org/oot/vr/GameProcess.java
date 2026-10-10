package org.oot.vr;

/**
 * Decides whether the process ends with MainActivity.
 *
 * Horizon OS keeps the process alive after the player quits, and the next launch puts a new
 * MainActivity in it. SDL_main then runs a second time over the static state of the first run.
 * libultraship registers its resource factories and console commands again. The second run uses
 * approximately two times the memory, and the system closes the app. Thus each launch must start
 * in a new process.
 *
 * This also applies when Android destroys the activity in the background without a quit: a new
 * process is the only safe way to start the game again.
 */
final class GameProcess {

    private GameProcess() {
    }

    /**
     * @param handedOverToSetup the activity ended in onCreate to start SetupActivity, which runs in
     *     this process and starts MainActivity again from it
     * @param changingConfigurations Android creates the activity again at once, in this process
     */
    static boolean endsWithActivity(boolean handedOverToSetup, boolean changingConfigurations) {
        return !handedOverToSetup && !changingConfigurations;
    }
}
