package org.oot.vr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

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
}
