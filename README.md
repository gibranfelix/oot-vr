# oot-vr

*The Legend of Zelda: Ocarina of Time* in first person on the Meta Quest 3S.
The game runs natively on the headset. You do not need a PC to play.

**Status: alpha.** You can play the main story from Kokiri Forest to Ganon's
Tower. Some problems are known. Read [`STATUS.md`](STATUS.md) before you play.

> [!IMPORTANT]
> You must own a legal copy of *The Legend of Zelda: Ocarina of Time*. To play,
> you make a dump of your own cartridge or disc.
>
> This repository and its releases do not contain game assets. We do not supply
> ROMs, and we do not help you get them. Do not ask for ROMs in the issues. Do
> not share ROMs in the issues.

## Features

- Stereo 3D. Each eye gets its own render pass.
- First-person camera at the eye height of Link. The camera follows your head.
- Motion controls. You swing the sword with your hand. You hold up the shield
  with your hand.
- Automatic world scale. The scale changes when Link changes from child to
  adult.
- Menus show on a panel that floats in front of you.

We tested the port only on the Meta Quest 3S.

## Install

You need a PC with `adb` for these steps. You do these steps one time only.

### 1. Make `oot.o2r` from your ROM

The game assets come from your ROM. The APK does not contain them.

1. Make a dump of your own cartridge or disc of Ocarina of Time. Use one of
   the supported versions below.
2. Build the host tools. Follow [`port/Android/README.md`](port/Android/README.md).
3. Put your `.z64` file in `port/OTRExporter/`.
4. Run `cmake --build port/build-host --target ExtractAssets`.
5. Get the file `port/oot.o2r`.

#### Supported versions

| Platform | Region | Versions |
|---|---|---|
| Nintendo 64 | Europe (PAL) | 1.0, 1.1 |
| Nintendo 64 | North America (NTSC-U) | 1.0, 1.1, 1.2 |
| Nintendo 64 | Japan (NTSC-J) | 1.0, 1.1, 1.2 |
| GameCube | Europe (PAL) | Ocarina of Time, Master Quest |
| GameCube | North America (NTSC-U) | Ocarina of Time, Master Quest |
| GameCube | Japan (NTSC-J) | Ocarina of Time, Master Quest, Collector's Edition |

To make sure that your dump is correct, compare its SHA-1 with the list in
[`port/docs/supportedHashes.json`](port/docs/supportedHashes.json). We tested
only the European GameCube version.

### 2. Install the APK

1. Download `oot-vr-<version>.apk` from
   [Releases](https://github.com/gibranfelix/oot-vr/releases).
2. Connect the headset to the PC with a USB cable.
3. Run these commands:

   ```
   adb install -r oot-vr-<version>.apk
   adb shell mkdir -p /sdcard/Android/data/org.oot.vr/files
   adb push oot.o2r /sdcard/Android/data/org.oot.vr/files/oot.o2r
   ```

4. Put on the headset.
5. Open **OoT VR** from **Unknown Sources**.

If the game does not start, read the "If the game does not start" section in
[`port/Android/README.md`](port/Android/README.md).

## Build from source

Read [`port/Android/README.md`](port/Android/README.md). One script builds the
APK: `port/Android/build-apk.sh`.

## Documentation

| File | Contents |
|---|---|
| [`STATUS.md`](STATUS.md) | What works, what does not work, where to help |
| [`docs/architecture.md`](docs/architecture.md) | How the VR layer connects to the game and the engine |
| [`CONTEXT.md`](CONTEXT.md) | The terms that this project uses |
| [`CONTRIBUTING.md`](CONTRIBUTING.md) | How to send changes |
| [`AGENTS.md`](AGENTS.md) | Rules for contributors and for AI agents |

## Credits

This port uses the work of other people:

- **[ShinyWindow](https://github.com/ShinyWindow)**:
  [`Shipwright-VR`](https://github.com/ShinyWindow/Shipwright-VR) and
  [`libultraship-vr`](https://github.com/ShinyWindow/libultraship-vr). The VR
  layer comes from these projects: the OpenXR session, the stereo render, the
  first-person camera, the physical combat, and many control fixes.
- **[Harbour Masters](https://github.com/HarbourMasters)**:
  [Ship of Harkinian](https://github.com/HarbourMasters/Shipwright), the port of
  Ocarina of Time that all of this uses.
- **[Kenix3](https://github.com/Kenix3)**:
  [`libultraship`](https://github.com/Kenix3/libultraship).
- **[linkzenic](https://github.com/linkzenic)**:
  [`Shipwright-Android`](https://github.com/linkzenic/Shipwright-Android), Ship
  of Harkinian on Android arm64 with GLES3.
- **[zeldaret](https://github.com/zeldaret/oot)**: the decompilation of Ocarina
  of Time.

## License

Not all of this repository has a license. The code that this project wrote is
MIT ([`LICENSE`](LICENSE)). Ship of Harkinian, its VR layer, and the Android
wrapper do not have a license. [`NOTICE.md`](NOTICE.md) gives the license of
each directory.

Zelda and Ocarina of Time are trademarks of Nintendo. This project has no
connection with Nintendo.
