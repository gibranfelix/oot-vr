package org.oot.vr;

/**
 * The on-device extractor: ZAPD and the Ship of Harkinian extractor, inside libsoh.so.
 *
 * SetupActivity is not an SDLActivity, so nothing has loaded the native libraries when it first
 * touches this class. They load here, with the same names and in the same order as
 * SDLActivity.getLibraries(). MainActivity keeps that default list, and SDL's JNI_OnLoad lives in
 * libSDL2.so, which libsoh.so needs. When the game starts later in the same process,
 * SDLActivity's own loadLibrary calls find the libraries already loaded and do nothing.
 *
 * The C++ side is soh/Extractor/AndroidRomExtractor.cpp.
 */
final class RomExtractor {

    static {
        System.loadLibrary("SDL2");
        System.loadLibrary("soh");
    }

    private RomExtractor() {
    }

    /**
     * Identifies a ROM without any dialog. Returns "ok &lt;zapdVersion&gt; &lt;0|1 for MQ&gt;" or
     * "error &lt;read|compressed|size|unsupported&gt;". Parse it with {@link RomCheck#parse}.
     */
    static native String nativeCheckRom(String romPath);

    /**
     * Makes oot.o2r (or oot-mq.o2r) in exportDir. Blocks for minutes: call it off the UI thread.
     * workDir must hold assets/ as {@link ExtractorAssets} lays it out, and TMPDIR must be set.
     * True only if the archive exists after the run. Once per process: ZAPD keeps global state.
     */
    static native boolean nativeExtract(String romPath, String workDir, String exportDir);

    /** {done, total} of the running extraction; {0, 0} before it starts. Safe from any thread. */
    static native long[] nativeProgress();
}
