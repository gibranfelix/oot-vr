# oot-vr

*The Legend of Zelda: Ocarina of Time* in first person on the Meta Quest 3S.
The game runs natively on the headset. You do not need a PC to play.

**Status: alpha.** You can play the main story from Kokiri Forest to Ganon's
Tower. Some problems are known. Read [`STATUS.md`](STATUS.md) before you play.

[![Watch the trailer on YouTube](docs/media/trailer.jpg)](https://youtu.be/gzuzNRE-Qyo)

All clips are recordings from a Meta Quest 3S.

| | |
|---|---|
| ![Link fights a Tektite with the sword and the shield](docs/media/tektite.webp) | ![Link shoots the bow from the back of Epona](docs/media/epona-bow.webp) |
| Sword and shield against a Tektite. | Bow on Epona. |
| ![Link holds the ocarina in Kokiri Forest](docs/media/ocarina.webp) | ![The pause screen on a panel that floats in front of the player](docs/media/floating-menu.webp) |
| Ocarina. | Pause screen. |
| ![Link fights Ganon with the sword](docs/media/ganon.webp) | ![The Great Fairy appears in front of Link](docs/media/great-fairy.webp) |
| Ganon. | Great Fairy. |

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

## Controls

These are the default controls. The right hand holds the sword. The left hand
holds the shield and uses the items.

| Control | Action |
|---|---|
| Left thumbstick | Move Link. |
| Right thumbstick ← → ↓ | Put the item of C-Left, C-Right, or C-Down in your hand. |
| Right thumbstick ↑ | Talk to Navi, when Navi has a message. |
| Right thumbstick click (hold) | Open the item selector. Move your hand to an item, then release the click. |
| Left trigger | Use the item in your hand. For the bow, hold to pull the string and release to shoot. |
| Right trigger | Z-target. |
| Both grips (hold) | Hold the sword and the shield. Release the grips to put them away. |
| A button | The N64 A button: action, talk, roll. |
| B button | The N64 B button. |
| Y button | Open or close the SoH menu. |
| Left Menu button | Start: open the pause screen. |
| Left grip | In the pause screen, go to the next page. |

To attack, swing the sword with your right hand. To block, hold the shield in
front of you with your left hand. To turn, turn your body.

The right thumbstick does not turn the view. To turn with the thumbstick,
enable **Artificial Turning** in VR Settings. Then the right thumbstick turns
the view and does not select items.

### SoH menu

The SoH menu contains VR Settings and the settings of Ship of Harkinian. Push
the Y button to open the menu on the floating panel. Push the Y button again to
close it. While the menu is open, the controls operate the menu, not Link:

| Control | Menu action |
|---|---|
| Left thumbstick | Move the selection. |
| A button | Accept or activate. |
| B button | Go back or cancel. |
| Left grip / right grip | Go to the previous tab / the next tab. |

The game does not stop while the menu is open. When you play the ocarina, the
Y button plays a note and does not open the menu.

### Ocarina

When you play the ocarina, the controls change:

| Note | Control |
|---|---|
| A (D4) | Right trigger |
| C-Down (F4) | Left trigger |
| C-Right (A4) | A button |
| C-Left (B4) | X button |
| C-Up (D5) | Y button |
| C-Up, C-Down, C-Left, C-Right | Right thumbstick ↑ ↓ ← → |
| Half step up / down | Right grip / left grip |
| Put the ocarina away | B button |

## Install

You need:

- a legal copy of Ocarina of Time: a cartridge or disc that you own;
- a PC with Windows, macOS, or Linux, to install the game;
- a USB-C cable for the headset.

You do these steps one time only.

### 1. Make a dump of your game

Make a dump of your own cartridge or disc. Use one of these versions:

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

### 2. Enable developer mode on the headset

The game does not come from the Meta store. Thus, the headset must be in
developer mode. Follow the steps from Meta:
[Device Setup](https://developers.meta.com/horizon/documentation/native/android/mobile-device-setup/).

### 3. Install the game with SideQuest

1. Install [SideQuest](https://sidequestvr.com/setup-howto) on your PC.
2. Connect the headset to the PC with the USB-C cable.
3. Put on the headset. Select **Always allow from this computer**, then
   select **Allow**.
4. Download `oot-vr-<version>.apk` from
   [Releases](https://github.com/gibranfelix/oot-vr/releases).
5. Drag the APK file into the SideQuest window. SideQuest installs it.
6. In SideQuest, open the file manager of the headset.
7. Copy your dump into the `Download` folder of the headset.

If you use `adb`, you can do steps 5 to 7 with these commands:

```
adb install -r oot-vr-<version>.apk
adb push <your-dump>.z64 /sdcard/Download/
```

### 4. Start the game and select your dump

1. Put on the headset.
2. Open the **Library**, and select **Unknown Sources**.
3. Open **OoT VR**. At the first start, a panel opens.
4. Select **Select ROM**. The file picker of the headset opens.
5. Go to the `Download` folder, and select your dump.
6. Wait until the extraction is complete. This takes some minutes. Do not
   remove the headset.
7. The game starts in VR.

The game makes the file `oot.o2r` from your dump, and then deletes its copy of
the dump. You can then delete your dump from the `Download` folder. The next
starts go directly to the game.

If the game does not start, read the "If the game does not start" section in
[`port/Android/README.md`](port/Android/README.md).

### Alternative: make `oot.o2r` on a PC

Use this method if the extraction on the headset does not work for you.

This port uses **Ship of Harkinian 9.2.3 "Ackbar Delta"**. Use this version.

1. Download Ship of Harkinian 9.2.3 for your PC from its
   [release page](https://github.com/HarbourMasters/Shipwright/releases/tag/9.2.3):
   `SoH-Ackbar-Delta-Win64.zip`, `SoH-Ackbar-Delta-Mac.zip`, or
   `SoH-Ackbar-Delta-Linux.zip`.
2. Extract the ZIP file and start Ship of Harkinian.
3. When Ship of Harkinian asks for a ROM, select your dump.
4. Wait until the extraction is complete. Then close Ship of Harkinian.
5. Find the file `oot.o2r`:
   - Windows and Linux: in the same folder as `soh.exe` or `soh.appimage`.
   - macOS: in `~/Library/Application Support/com.shipofharkinian.soh/`.
6. With the SideQuest file manager, copy `oot.o2r` into
   `Android/data/org.oot.vr/files/` on the headset. Make the folders if they do
   not exist.

When `oot.o2r` is on the headset, the game does not open the panel.

## Build from source

Read [`port/Android/README.md`](port/Android/README.md). One script builds the
APK: `port/Android/build-apk.sh`. The same document tells how to make
`oot.o2r` with the tools of this repository, without Ship of Harkinian for PC.

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
  Ocarina of Time that all of this uses. This port is based on version 9.2.3
  "Ackbar Delta".
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
