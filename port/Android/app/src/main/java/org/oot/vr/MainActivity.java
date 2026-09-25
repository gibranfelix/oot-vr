package org.oot.vr;

import android.app.PendingIntent;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

import org.libsdl.app.SDLActivity;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * Quest entry point.
 *
 * Deliberately thin. linkzenic's phone port carries ~1500 lines here because it has to be a
 * self-service app: an on-device ROM picker, an extraction progress dialog, and an on-screen
 * touch overlay, all wired to thirteen JNI methods.
 *
 * Here the ROM picker and the extraction are not in this activity at all. When oot.o2r does not
 * exist yet, this activity hands over to SetupActivity, a 2D panel, before SDL can start, and ends.
 * SetupActivity copies the player's ROM into the cache, makes oot.o2r from it, deletes the copy,
 * and starts this activity again. VR thus never runs without the archive, and the OpenXR session
 * never has to come back from a system window. Input arrives through OpenXR actions, not a
 * touchscreen. What is left here is: make sure soh.o2r is where the game looks for it, send a first
 * start to the setup, and hand over to SDL.
 *
 * No storage permission either. libultraship locates its data with
 * SDL_AndroidGetExternalStoragePath(), which is the app-private external directory - readable and
 * writable with no permission at all, and adb-reachable. The reference port needs
 * MANAGE_EXTERNAL_STORAGE only because it hardcodes /storage/emulated/0/SOH and then has to patch
 * libultraship to make that configurable. Nothing here has to. The ROM needs no permission either:
 * the system picker grants access to the one file the player selects.
 */
public class MainActivity extends SDLActivity {

    private static final String TAG = "OoTVR";
    /** Home opens this PendingIntent as a panel. From Meta's "Hybrid apps overview" guide. */
    private static final String EXTRA_LAUNCH_IN_HOME = "extra_launch_in_home_pending_intent";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // Must match SDL_AndroidGetExternalStoragePath(), which is what libultraship asks for:
        //   /sdcard/Android/data/org.oot.vr/files
        File root = getExternalFilesDir(null);
        if (root == null) {
            Log.e(TAG, "External files dir unavailable");
        } else {
            copyAssetIfMissing("soh.o2r", new File(root, "soh.o2r"));
            // RunExtract() checks for an assets/ directory on EVERY launch, before it looks at
            // whether an archive is already present - and if it is missing it registers a modal
            // and spins in its own draw loop waiting for an OK that a VR build can never deliver.
            // The directory has to exist even when this start only hands over to the setup.
            copyAssetTree("assets", new File(root, "assets"));
        }
        // Android requires super.onCreate even from an activity that ends at once. SDLActivity's
        // onCreate only loads the libraries and builds the surface. SDL_main starts on the first
        // onResume with focus and a ready surface, and an activity that finishes inside onCreate
        // gets no onResume: it goes straight to onDestroy, whose nativeQuit() only frees what
        // onCreate made. A later MainActivity in the same process makes them again.
        super.onCreate(savedInstanceState);
        if (root != null && SetupGate.needsSetup(root)) {
            Log.i(TAG, "No game archive: starting the setup");
            startSetup();
            finishAndRemoveTask();
        }
    }

    /**
     * Immersive to panel, as Meta's hybrid-app guide does it: ask Home to open the panel. Started
     * directly from an immersive activity, a panel opens as an overlay on top of that activity,
     * which is about to end. The direct start remains as a fallback for a system without that Home.
     */
    private void startSetup() {
        Intent setup = new Intent(this, SetupActivity.class);
        setup.setAction(Intent.ACTION_MAIN);
        setup.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        PendingIntent pending = PendingIntent.getActivity(this, 0, setup,
            PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);
        Intent home = new Intent(Intent.ACTION_MAIN)
            .addCategory(Intent.CATEGORY_HOME)
            .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            .putExtra(EXTRA_LAUNCH_IN_HOME, pending);
        try {
            startActivity(home);
        } catch (ActivityNotFoundException e) {
            Log.w(TAG, "No Home to open the setup panel in; starting it directly");
            startActivity(setup);
        }
    }

    /** Recursively unpack an APK asset directory, skipping anything already on disk. */
    private void copyAssetTree(String assetDir, File target) {
        try {
            String[] entries = getAssets().list(assetDir);
            if (entries == null || entries.length == 0) {
                return; // a file, not a directory
            }
            if (!target.exists() && !target.mkdirs()) {
                Log.e(TAG, "Could not create " + target);
                return;
            }
            for (String entry : entries) {
                String childAsset = assetDir + "/" + entry;
                File childTarget = new File(target, entry);
                String[] grandchildren = getAssets().list(childAsset);
                if (grandchildren != null && grandchildren.length > 0) {
                    copyAssetTree(childAsset, childTarget);
                } else {
                    copyAssetIfMissing(childAsset, childTarget);
                }
            }
        } catch (IOException e) {
            Log.w(TAG, "Could not unpack " + assetDir + " (" + e.getMessage() + ")");
        }
    }

    /**
     * soh.o2r holds the PORT's assets and rides inside the APK; oot.o2r holds the GAME's and never
     * does: SetupActivity makes it on the headset, or the player pushes one made on a PC. Copying
     * only when absent keeps a hand-pushed replacement from being overwritten on every launch.
     */
    private void copyAssetIfMissing(String assetName, File target) {
        if (target.exists()) {
            return;
        }
        try (InputStream in = getAssets().open(assetName);
             OutputStream out = new FileOutputStream(target)) {
            byte[] buf = new byte[1 << 16];
            int n;
            while ((n = in.read(buf)) > 0) {
                out.write(buf, 0, n);
            }
            Log.i(TAG, "Copied " + assetName + " to " + target);
        } catch (IOException e) {
            // Not fatal: the build may simply not bundle it yet, and the game reports the miss.
            Log.w(TAG, "No bundled " + assetName + " (" + e.getMessage() + ")");
        }
    }
}
