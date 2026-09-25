package org.oot.vr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;

import org.junit.Test;

public class ExtractorAssetsTest {

    private static ExtractorAssets.Copy copy(String asset, String target) {
        return new ExtractorAssets.Copy(asset, target);
    }

    @Test
    public void rootsAreTheExtractorDataAndOneVersionOfXml() {
        assertEquals(
            Arrays.asList("assets/extractor", "extractor-xml/GC_NMQ_PAL_F"),
            ExtractorAssets.roots("GC_NMQ_PAL_F"));
    }

    @Test
    public void extractorDataIsFlattenedIntoAssets() {
        List<ExtractorAssets.Copy> plan = ExtractorAssets.plan("GC_NMQ_PAL_F", Arrays.asList(
            "assets/extractor/Config_GC_NMQ_PAL_F.xml",
            "assets/extractor/TexturePool.xml",
            "assets/extractor/filelists/gamecube_pal.txt",
            "assets/extractor/symbols/SymbolMap_OoTMqDbg.txt"));
        assertEquals(Arrays.asList(
            copy("assets/extractor/Config_GC_NMQ_PAL_F.xml", "assets/Config_GC_NMQ_PAL_F.xml"),
            copy("assets/extractor/TexturePool.xml", "assets/TexturePool.xml"),
            copy("assets/extractor/filelists/gamecube_pal.txt", "assets/filelists/gamecube_pal.txt"),
            copy("assets/extractor/symbols/SymbolMap_OoTMqDbg.txt", "assets/symbols/SymbolMap_OoTMqDbg.txt")),
            plan);
    }

    @Test
    public void xmlOfTheVersionGoesUnderAssetsXml() {
        List<ExtractorAssets.Copy> plan = ExtractorAssets.plan("GC_NMQ_PAL_F", Arrays.asList(
            "extractor-xml/GC_NMQ_PAL_F/objects/gameplay_keep.xml",
            "extractor-xml/GC_NMQ_PAL_F/scenes/dungeons/ydan.xml"));
        assertEquals(Arrays.asList(
            copy("extractor-xml/GC_NMQ_PAL_F/objects/gameplay_keep.xml",
                 "assets/xml/GC_NMQ_PAL_F/objects/gameplay_keep.xml"),
            copy("extractor-xml/GC_NMQ_PAL_F/scenes/dungeons/ydan.xml",
                 "assets/xml/GC_NMQ_PAL_F/scenes/dungeons/ydan.xml")),
            plan);
    }

    @Test
    public void xmlOfOtherVersionsIsSkipped() {
        List<ExtractorAssets.Copy> plan = ExtractorAssets.plan("GC_NMQ_PAL_F", Arrays.asList(
            "extractor-xml/GC_MQ_D/objects/gameplay_keep.xml",
            // A version whose name starts with the selected one must not match by prefix.
            "extractor-xml/GC_NMQ_PAL_F_X/objects/gameplay_keep.xml",
            "extractor-xml/GC_NMQ_PAL_F/objects/gameplay_keep.xml"));
        assertEquals(Collections.singletonList(
            copy("extractor-xml/GC_NMQ_PAL_F/objects/gameplay_keep.xml",
                 "assets/xml/GC_NMQ_PAL_F/objects/gameplay_keep.xml")),
            plan);
    }

    @Test
    public void unrelatedAssetsAreSkipped() {
        // soh.o2r ships in the APK too; it is not extractor input.
        List<ExtractorAssets.Copy> plan = ExtractorAssets.plan("GC_NMQ_PAL_F", Arrays.asList(
            "soh.o2r", "assets/other/file.txt", "assets/extractor"));
        assertTrue(plan.isEmpty());
    }

    @Test
    public void planWithoutXmlOfTheVersionIsIncomplete() {
        // The APK was built without the XML staging, or without this version.
        List<ExtractorAssets.Copy> plan = ExtractorAssets.plan("GC_NMQ_PAL_F", Arrays.asList(
            "assets/extractor/Config_GC_NMQ_PAL_F.xml",
            "extractor-xml/GC_MQ_D/objects/gameplay_keep.xml"));
        assertFalse(ExtractorAssets.hasXml(plan));
    }

    @Test
    public void planWithXmlOfTheVersionIsComplete() {
        List<ExtractorAssets.Copy> plan = ExtractorAssets.plan("GC_NMQ_PAL_F", Arrays.asList(
            "assets/extractor/Config_GC_NMQ_PAL_F.xml",
            "extractor-xml/GC_NMQ_PAL_F/objects/gameplay_keep.xml"));
        assertTrue(ExtractorAssets.hasXml(plan));
    }

    @Test(expected = IllegalArgumentException.class)
    public void pathWithParentSegmentIsRejected() {
        ExtractorAssets.plan("GC_NMQ_PAL_F",
            Collections.singletonList("assets/extractor/../../evil.xml"));
    }

    @Test(expected = IllegalArgumentException.class)
    public void pathWithEmptySegmentIsRejected() {
        ExtractorAssets.plan("GC_NMQ_PAL_F",
            Collections.singletonList("extractor-xml/GC_NMQ_PAL_F//a.xml"));
    }

    @Test(expected = IllegalArgumentException.class)
    public void versionThatIsNotASingleNameIsRejected() {
        ExtractorAssets.roots("../GC_NMQ_PAL_F");
    }
}
