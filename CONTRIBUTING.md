# Contributing

Thank you for your help. This is a small project. We examine each pull request
when we have time.

## Before you start

1. Read [`STATUS.md`](STATUS.md) and [`docs/architecture.md`](docs/architecture.md).
2. Read [`AGENTS.md`](AGENTS.md). Its rules apply to all contributors.
3. Find an issue, or open a new one. For a large change, open an issue first.

## Report a bug

Use the bug report template. Give this data:

- the headset model and the Horizon OS version;
- the release or the commit that you use;
- the location in the game (scene, room, or cutscene);
- the steps to cause the problem;
- a log from `adb logcat -s soh:V`, if possible.

Do not attach ROMs, `oot.o2r`, or other game files. Do not tell where you got
your ROM.

## Send a pull request

1. Make one change in each pull request.
2. Write the commit messages in English.
3. Write the documentation in ASD-STE100 Simplified Technical English.
4. Build the APK with `port/Android/build-apk.sh`.
5. Test the change on a headset. In the pull request, tell what you tested and
   on which headset.
6. If you cannot test on a headset, say so in the pull request.

An automatic review can add comments to your pull request. A maintainer does
the final review. Only a maintainer can merge.

## Test a pull request on a headset

Each pull request makes a debug APK. You do not need a local build to test it.

1. Open the pull request.
2. Open the **Checks** tab.
3. Select the **Build** workflow.
4. In **Artifacts**, download `oot-vr-debug-pr<number>-<commit>.apk`.
5. Connect the headset to the computer with a USB cable.
6. Install the APK with `adb install oot-vr-debug-pr<number>-<commit>.apk`.

The debug APK has a signature that is different from the release APK. If the
release version is on the headset, `adb install` stops with a signature error.
Remove the release version first with `adb uninstall org.oot.vr`.
The `adb uninstall` command also deletes your save data and `oot.o2r`. The app
keeps them in `/sdcard/Android/data/org.oot.vr/files`. Copy this directory to
the computer before you remove the release version:

```
adb pull /sdcard/Android/data/org.oot.vr/files oot-vr-files
```

All debug APKs have the same signature. A debug APK installs over a different
debug APK with `adb install -r`, and your save data stays. A local debug build
from before 2026-10-02 has a different signature. Remove it one time.

GitHub keeps each debug APK for 14 days. A push to `main` also makes a debug
APK, with the name `oot-vr-debug-main-<commit>.apk`.

## License

When you send a pull request, you agree that your change is under the MIT
license of this project. Read [`LICENSE`](LICENSE) and [`NOTICE.md`](NOTICE.md).
