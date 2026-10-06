package org.oot.vr;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * The verdict of {@link RomExtractor#nativeCheckRom} on a selected file.
 *
 * The native side answers with one short line rather than an object so the JNI surface stays a
 * single String. Everything that can go wrong in reading that line collapses to
 * {@link Error#UNKNOWN}: the setup must show some message and offer another file, never crash on a
 * reply it did not expect.
 *
 * The version is checked here, not trusted, because {@link ExtractorAssets} turns it into a
 * directory name.
 */
final class RomCheck {

    /**
     * The versions the panel names when it rejects a ROM. A host test keeps this list equal to the
     * table in the README. A ROM with a patch, for example a fan translation, has a different hash
     * and the extractor does not accept it.
     */
    static final List<String> SUPPORTED_VERSIONS = Collections.unmodifiableList(Arrays.asList(
        "Nintendo 64, Europe (PAL): 1.0, 1.1",
        "Nintendo 64, North America (NTSC-U): 1.0, 1.1, 1.2",
        "Nintendo 64, Japan (NTSC-J): 1.0, 1.1, 1.2",
        "GameCube, Europe (PAL): Ocarina of Time, Master Quest",
        "GameCube, North America (NTSC-U): Ocarina of Time, Master Quest",
        "GameCube, Japan (NTSC-J): Ocarina of Time, Master Quest, Collector's Edition"));

    /** Why a file cannot be used. Each one has the text the panel shows for it. */
    enum Error {
        READ("The file could not be read. Select the file again."),
        COMPRESSED("This ROM is compressed. Select a dump that is not compressed."),
        SIZE("The size of this file is not the size of an Ocarina of Time ROM. "
            + "Select another file."),
        UNSUPPORTED("This ROM is modified, or this version of Ocarina of Time is not supported. "
            + "Select a dump without patches of one of these versions:\n"
            + String.join("\n", SUPPORTED_VERSIONS)),
        UNKNOWN("The ROM could not be checked. Select another file.");

        private final String message;

        Error(String message) {
            this.message = message;
        }

        String message() {
            return message;
        }
    }

    /** Same alphabet as the Config_<version>.xml names; nothing that can walk out of a directory. */
    static final Pattern VERSION = Pattern.compile("[A-Z0-9_]+");

    private static final Pattern OK = Pattern.compile("ok ([^ ]+) ([01])");
    private static final Pattern ERROR = Pattern.compile("error ([a-z]+)");

    private final String zapdVersion;
    private final boolean masterQuest;
    private final Error error;

    private RomCheck(String zapdVersion, boolean masterQuest, Error error) {
        this.zapdVersion = zapdVersion;
        this.masterQuest = masterQuest;
        this.error = error;
    }

    static RomCheck failed(Error error) {
        return new RomCheck(null, false, error);
    }

    static RomCheck parse(String result) {
        if (result == null) {
            return failed(Error.UNKNOWN);
        }
        String line = result.trim();
        Matcher ok = OK.matcher(line);
        if (ok.matches()) {
            String version = ok.group(1);
            if (!VERSION.matcher(version).matches()) {
                return failed(Error.UNKNOWN);
            }
            return new RomCheck(version, ok.group(2).equals("1"), null);
        }
        Matcher err = ERROR.matcher(line);
        if (err.matches()) {
            switch (err.group(1)) {
                case "read":        return failed(Error.READ);
                case "compressed":  return failed(Error.COMPRESSED);
                case "size":        return failed(Error.SIZE);
                case "unsupported": return failed(Error.UNSUPPORTED);
                default:            return failed(Error.UNKNOWN);
            }
        }
        return failed(Error.UNKNOWN);
    }

    boolean isOk() {
        return error == null;
    }

    /** The ZAPD version name, for example GC_NMQ_PAL_F; null when the check failed. */
    String zapdVersion() {
        return zapdVersion;
    }

    boolean isMasterQuest() {
        return masterQuest;
    }

    /** Null when the check passed. */
    Error error() {
        return error;
    }

    /** The text for the panel; null when the check passed. */
    String message() {
        return error == null ? null : error.message();
    }
}
