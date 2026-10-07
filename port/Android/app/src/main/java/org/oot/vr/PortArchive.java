package org.oot.vr;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;

/**
 * Decides whether soh.o2r must be copied again from the APK.
 *
 * soh.o2r holds the PORT's assets (shaders, fonts, port textures) and changes with the APK. A copy
 * only when the file is missing kept the archive of the first install forever, so an update never
 * reached the headset. A stamp file next to the archive holds the APK update time of the last copy:
 * when the APK changes, the archive is copied again.
 */
final class PortArchive {

    private static final String STAMP_SUFFIX = ".apk-time";

    private PortArchive() {
    }

    static File stampFile(File archive) {
        return new File(archive.getPath() + STAMP_SUFFIX);
    }

    static boolean needsCopy(File archive, long apkUpdateTime) {
        if (!archive.isFile() || archive.length() == 0) {
            return true;
        }
        File stamp = stampFile(archive);
        if (!stamp.isFile()) {
            return true;
        }
        try {
            String text = new String(Files.readAllBytes(stamp.toPath()), StandardCharsets.UTF_8).trim();
            return Long.parseLong(text) != apkUpdateTime;
        } catch (IOException | NumberFormatException e) {
            return true;
        }
    }

    static void writeStamp(File archive, long apkUpdateTime) throws IOException {
        try (FileOutputStream out = new FileOutputStream(stampFile(archive))) {
            out.write(Long.toString(apkUpdateTime).getBytes(StandardCharsets.UTF_8));
        }
    }
}
