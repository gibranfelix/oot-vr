package org.oot.vr;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;

public class PortArchiveTest {

    private static final long APK_TIME = 1_790_000_000_000L;

    @Rule
    public TemporaryFolder tmp = new TemporaryFolder();

    private File archive() {
        return new File(tmp.getRoot(), "soh.o2r");
    }

    private static void write(File f, int bytes) throws IOException {
        try (FileOutputStream out = new FileOutputStream(f)) {
            out.write(new byte[bytes]);
        }
    }

    @Test
    public void missingArchiveNeedsCopy() {
        assertTrue(PortArchive.needsCopy(archive(), APK_TIME));
    }

    @Test
    public void emptyArchiveNeedsCopy() throws IOException {
        write(archive(), 0);
        PortArchive.writeStamp(archive(), APK_TIME);
        assertTrue(PortArchive.needsCopy(archive(), APK_TIME));
    }

    @Test
    public void archiveFromAnOlderInstallNeedsCopy() throws IOException {
        // An install before this change wrote no stamp.
        write(archive(), 16);
        assertTrue(PortArchive.needsCopy(archive(), APK_TIME));
    }

    @Test
    public void archiveFromThisApkNeedsNoCopy() throws IOException {
        write(archive(), 16);
        PortArchive.writeStamp(archive(), APK_TIME);
        assertFalse(PortArchive.needsCopy(archive(), APK_TIME));
    }

    @Test
    public void updatedApkNeedsCopy() throws IOException {
        write(archive(), 16);
        PortArchive.writeStamp(archive(), APK_TIME);
        assertTrue(PortArchive.needsCopy(archive(), APK_TIME + 1));
    }

    @Test
    public void unreadableStampNeedsCopy() throws IOException {
        write(archive(), 16);
        try (FileOutputStream out = new FileOutputStream(PortArchive.stampFile(archive()))) {
            out.write("not a number".getBytes());
        }
        assertTrue(PortArchive.needsCopy(archive(), APK_TIME));
    }
}
