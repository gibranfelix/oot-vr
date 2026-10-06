package org.oot.vr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.List;

import org.junit.Test;

public class RomCheckTest {

    @Test
    public void okResultCarriesVersionAndNoMessage() {
        RomCheck check = RomCheck.parse("ok GC_NMQ_PAL_F 0");
        assertTrue(check.isOk());
        assertEquals("GC_NMQ_PAL_F", check.zapdVersion());
        assertFalse(check.isMasterQuest());
        assertNull(check.error());
        assertNull(check.message());
    }

    @Test
    public void okResultReportsMasterQuest() {
        RomCheck check = RomCheck.parse("ok GC_MQ_D 1");
        assertTrue(check.isOk());
        assertTrue(check.isMasterQuest());
    }

    @Test
    public void surroundingWhitespaceIsIgnored() {
        assertTrue(RomCheck.parse("  ok N64_PAL_11 0\n").isOk());
    }

    @Test
    public void eachErrorCodeMapsToItsOwnError() {
        assertEquals(RomCheck.Error.READ, RomCheck.parse("error read").error());
        assertEquals(RomCheck.Error.COMPRESSED, RomCheck.parse("error compressed").error());
        assertEquals(RomCheck.Error.SIZE, RomCheck.parse("error size").error());
        assertEquals(RomCheck.Error.UNSUPPORTED, RomCheck.parse("error unsupported").error());
    }

    @Test
    public void everyErrorHasADistinctMessage() {
        RomCheck.Error[] errors = RomCheck.Error.values();
        for (RomCheck.Error a : errors) {
            assertNotNull(a.message());
            assertFalse(a.message().isEmpty());
            for (RomCheck.Error b : errors) {
                if (a != b) {
                    assertNotEquals(a.message(), b.message());
                }
            }
        }
    }

    @Test
    public void errorResultHasNoVersionAndCarriesTheMessage() {
        RomCheck check = RomCheck.parse("error compressed");
        assertFalse(check.isOk());
        assertNull(check.zapdVersion());
        assertEquals(RomCheck.Error.COMPRESSED.message(), check.message());
    }

    @Test
    public void unknownErrorCodeIsUnknown() {
        assertEquals(RomCheck.Error.UNKNOWN, RomCheck.parse("error sideways").error());
    }

    @Test
    public void malformedResultsAreUnknownErrors() {
        String[] bad = {
            null, "", "ok", "ok GC_NMQ_PAL_F", "ok GC_NMQ_PAL_F 2", "ok GC_NMQ_PAL_F 0 extra",
            "error", "error read extra", "OK GC_NMQ_PAL_F 0", "hello",
        };
        for (String s : bad) {
            RomCheck check = RomCheck.parse(s);
            assertFalse("accepted: " + s, check.isOk());
            assertEquals("for: " + s, RomCheck.Error.UNKNOWN, check.error());
        }
    }

    @Test
    public void versionThatCouldEscapeAPathIsRejected() {
        // The version becomes a directory name under the extraction work dir.
        String[] bad = {"ok ../x 0", "ok a/b 0", "ok GC.NMQ 0", "ok gc_nmq 0"};
        for (String s : bad) {
            assertEquals("for: " + s, RomCheck.Error.UNKNOWN, RomCheck.parse(s).error());
        }
    }

    @Test
    public void unsupportedMessageAsksForADumpWithoutPatches() {
        String text = RomCheck.Error.UNSUPPORTED.message();
        assertTrue(text, text.contains("modified"));
        assertTrue(text, text.contains("without patches"));
    }

    @Test
    public void unsupportedMessageListsEachSupportedVersion() {
        String text = RomCheck.Error.UNSUPPORTED.message();
        for (String line : RomCheck.SUPPORTED_VERSIONS) {
            assertTrue("missing: " + line, text.contains(line));
        }
    }

    @Test
    public void supportedVersionsAreTheVersionsInTheReadme() throws IOException {
        List<String> readme = new ArrayList<>();
        boolean inSection = false;
        for (String line : Files.readAllLines(readme().toPath(), StandardCharsets.UTF_8)) {
            if (line.startsWith("### ")) {
                inSection = line.equals("### 1. Make a dump of your game");
                continue;
            }
            if (!inSection || !line.startsWith("|") || line.contains("---")) {
                continue;
            }
            String[] cells = line.split("\\|");
            String platform = cells[1].trim();
            if (platform.equals("Platform")) {
                continue;
            }
            readme.add(platform + ", " + cells[2].trim() + ": " + cells[3].trim());
        }
        assertFalse("no version table in the README", readme.isEmpty());
        assertEquals(readme, RomCheck.SUPPORTED_VERSIONS);
    }

    /** The README of the repository; the tests run from port/Android/app. */
    private static File readme() {
        for (File dir = new File(System.getProperty("user.dir")).getAbsoluteFile(); dir != null;
                dir = dir.getParentFile()) {
            File file = new File(dir, "README.md");
            if (file.isFile() && new File(dir, ".github/soh-version").isFile()) {
                return file;
            }
        }
        throw new AssertionError("README.md of the repository not found");
    }
}
