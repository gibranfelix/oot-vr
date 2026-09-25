# Android wrapper (Quest 3S)

## Requirements

- Linux x86_64. We did not test other systems.
- A Linux file system for the checkout. The build needs symlinks and the
  execute bit.
- CMake and Ninja.
- The host build dependencies of Ship of Harkinian. Read
  [`port/linux-build-deps/`](../linux-build-deps/).
- JDK 17.
- The Android SDK in `$HOME/Android/Sdk`, or set `ANDROID_HOME`.
- The Android NDK `26.3.11579264`, or set `ANDROID_NDK_HOME`.

## Build the APK

Run:

```
port/Android/build-apk.sh
```

The script does these steps:

1. **Host build.** It makes `soh.o2r`, the **port** assets: shaders, logo, and
   menu textures. This step needs ZAPD on the PC. Thus, it is a separate build.
   The script skips this step if `port/soh.o2r` exists.
2. **arm64 build** of the game. CMake downloads the OpenXR loader from Maven and
   checks its hash.
3. **Copy.** It copies `libsoh.so`, `libSDL2.so`, and `libopenxr_loader.so` to
   `app/libs/arm64-v8a/`. It removes the debug symbols. It copies `soh.o2r` to
   the assets.
4. **Package.** Gradle makes the APK. Gradle does not run CMake. The native
   libraries are ready before this step.

The APK is at `port/Android/app/build/outputs/apk/debug/app-debug.apk`.

## Make the game assets: `oot.o2r`

The game assets come from **your** legal copy of the game: a dump of a
cartridge or disc that you own. They **never** go into the repository or into
the APK.

Players can make `oot.o2r` with Ship of Harkinian 9.2.3 for PC. Read the main
[`README.md`](../../README.md). As a developer, you can also make it with the
tools of this repository:

1. Put your `.z64` file in `port/OTRExporter/`. The `.gitignore` blocks this
   file.
2. Run:

   ```
   cmake --build port/build-host --target ExtractAssets
   ```

3. Get the file `port/oot.o2r`.

## Install on the headset

1. Connect the headset to the PC with a USB cable.
2. Run:

   ```
   adb install -r port/Android/app/build/outputs/apk/debug/app-debug.apk
   adb shell mkdir -p /sdcard/Android/data/org.oot.vr/files
   adb push port/oot.o2r /sdcard/Android/data/org.oot.vr/files/oot.o2r
   ```

3. Put on the headset.
4. Open **OoT VR** from **Unknown Sources**.

## If the game does not start

Usually, the cause is not the port. Horizon OS stops the launch and shows no
message in these conditions:

- **Nobody wears the headset.** `vrlockscreen/.SensorLockActivity` goes to the
  foreground, and Horizon OS keeps the launch for later. You cannot change this
  with `adb`.
- **A system dialog is open** in the headset.

Before you debug, run:

```
adb shell dumpsys activity activities | grep topResumedActivity
```

If the output does not show `org.oot.vr`, the cause is not the code.

## See what the game does

```
adb logcat -s soh:V                 # the full game log; VR messages start with [VR]
adb exec-out screencap -p > s.png   # the left eye, through the mirror (not tested)
```

For more debug tools, read [`docs/architecture.md`](../../docs/architecture.md).
