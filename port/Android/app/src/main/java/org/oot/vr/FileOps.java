package org.oot.vr;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/** File work for the setup, free of Android types so the host tests can run it. */
final class FileOps {

    /** The stream held more bytes than the caller allowed. */
    static final class TooLargeException extends IOException {
        TooLargeException(long maxBytes) {
            super("More than " + maxBytes + " bytes");
        }
    }

    private FileOps() {
    }

    /**
     * Copies a stream to a file.
     *
     * The bytes go to target.part first and are renamed at the end, so a copy that fails or is cut
     * short never leaves a file with the final name. The limit exists because the picker accepts
     * any file: selecting a video by mistake must not fill the headset's storage before the size
     * check can refuse it.
     */
    static void copy(InputStream in, File target, long maxBytes) throws IOException {
        File parent = target.getAbsoluteFile().getParentFile();
        if (parent != null && !parent.isDirectory() && !parent.mkdirs()) {
            throw new IOException("Could not create " + parent);
        }
        File part = new File(target.getPath() + ".part");
        boolean done = false;
        try {
            try (OutputStream out = new FileOutputStream(part)) {
                byte[] buf = new byte[1 << 16];
                long total = 0;
                int n;
                while ((n = in.read(buf)) > 0) {
                    total += n;
                    if (total > maxBytes) {
                        throw new TooLargeException(maxBytes);
                    }
                    out.write(buf, 0, n);
                }
            }
            if (target.exists() && !target.delete()) {
                throw new IOException("Could not replace " + target);
            }
            if (!part.renameTo(target)) {
                throw new IOException("Could not rename " + part);
            }
            done = true;
        } finally {
            if (!done) {
                part.delete();
            }
        }
    }

    /** Deletes a file or a directory with all its contents. True when nothing is left. */
    static boolean deleteTree(File f) {
        File[] children = f.listFiles();
        if (children != null) {
            for (File child : children) {
                deleteTree(child);
            }
        }
        return f.delete() || !f.exists();
    }
}
