package org.oot.vr;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Objects;

/**
 * Which APK assets the extractor needs, and where each one goes under its work directory.
 *
 * The extractor reads everything from one assets/ directory: the Config_<version>.xml files, the
 * file lists, the symbols and TexturePool.xml at its top, and the ZAPD XML of the version under
 * xml/<version>/. The APK keeps the two apart. The configuration files sit in
 * assets/extractor/ because MainActivity copies the APK's assets/ tree to the device on every fresh
 * install. The XML sits in extractor-xml/ so that this copy does not take 54 MB of files for all
 * versions. Only the XML of the one version the ROM needs is copied here, and only for the time of
 * the extraction.
 *
 * Every target is relative to the work directory. A path that could step out of it is an error,
 * not a skip: it can only come from a broken APK.
 */
final class ExtractorAssets {

    static final String EXTRACTOR_ROOT = "assets/extractor";
    static final String XML_ROOT = "extractor-xml";

    private ExtractorAssets() {
    }

    /** One file to copy: APK asset path, and target path relative to the work directory. */
    static final class Copy {
        final String asset;
        final String target;

        Copy(String asset, String target) {
            this.asset = asset;
            this.target = target;
        }

        @Override
        public boolean equals(Object o) {
            if (!(o instanceof Copy)) {
                return false;
            }
            Copy c = (Copy) o;
            return asset.equals(c.asset) && target.equals(c.target);
        }

        @Override
        public int hashCode() {
            return Objects.hash(asset, target);
        }

        @Override
        public String toString() {
            return asset + " -> " + target;
        }
    }

    /** The APK asset directories to list for a version. Listing all of extractor-xml is slow. */
    static List<String> roots(String zapdVersion) {
        checkVersion(zapdVersion);
        return Arrays.asList(EXTRACTOR_ROOT, XML_ROOT + "/" + zapdVersion);
    }

    /** @param assetFiles APK asset paths of files (not directories), in any order. */
    static List<Copy> plan(String zapdVersion, List<String> assetFiles) {
        checkVersion(zapdVersion);
        String extractorPrefix = EXTRACTOR_ROOT + "/";
        String xmlPrefix = XML_ROOT + "/" + zapdVersion + "/";
        List<Copy> plan = new ArrayList<>();
        for (String asset : assetFiles) {
            String target;
            if (asset.startsWith(extractorPrefix)) {
                target = "assets/" + asset.substring(extractorPrefix.length());
            } else if (asset.startsWith(xmlPrefix)) {
                target = "assets/xml/" + zapdVersion + "/" + asset.substring(xmlPrefix.length());
            } else {
                continue;
            }
            checkPath(asset);
            plan.add(new Copy(asset, target));
        }
        return plan;
    }

    /** False when the APK has no XML for the version: the extraction cannot succeed. */
    static boolean hasXml(List<Copy> plan) {
        for (Copy copy : plan) {
            if (copy.target.startsWith("assets/xml/")) {
                return true;
            }
        }
        return false;
    }

    private static void checkVersion(String zapdVersion) {
        if (zapdVersion == null || !RomCheck.VERSION.matcher(zapdVersion).matches()) {
            throw new IllegalArgumentException("Bad version: " + zapdVersion);
        }
    }

    private static void checkPath(String path) {
        for (String segment : path.split("/", -1)) {
            if (segment.isEmpty() || segment.equals(".") || segment.equals("..")) {
                throw new IllegalArgumentException("Bad asset path: " + path);
            }
        }
    }
}
