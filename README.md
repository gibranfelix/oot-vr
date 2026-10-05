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
front of you with your left hand.

### Control schemes

At the first start, the game asks how you want to turn. You can change the
answer in VR Settings > VR Inputs > **Control Scheme**:

| Scheme | How you turn | Right thumbstick |
|---|---|---|
| Default | Turn your body. | Put the item of C-Left, C-Right, or C-Down in your hand. |
| Shipwright-VR | Push the right thumbstick. The view turns smoothly. | Turns the view. Use the item selector to take items. |

The two schemes use the same buttons. The Shipwright-VR scheme comes from the
PCVR mod [Shipwright-VR](https://github.com/ShinyWindow/Shipwright-VR).

When you change a binding or a turn setting, Control Scheme shows **Custom**.
Select **Reset to Scheme** to go back to the scheme.

### SoH menu

The SoH menu contains VR Settings and the settings of Ship of Harkinian. Push
the Y button to open the menu on the floating panel. Push the Y button again to
close it. While the menu is open, the controls operate the menu, not Link:

| Control | Menu action |
|---|---|
| Point a controller at the panel | Move the pointer. |
| Trigger of the hand that points | Click. |
| Right thumbstick ↑ ↓ | Scroll, when the right hand points. |
| Left thumbstick | Move the selection. |
| A button | Accept or activate. |
| B button | Go back or cancel. |
| Left grip / right grip | Go to the previous tab / the next tab. |

A beam shows where the controller points. The right hand points first. The left
hand points when the right hand does not point at the panel.

The game does not stop while the menu is open. The Y button also opens the
menu when you play the ocarina. In the classic control
scheme, C-Right is on the left Menu button because the Y button opens the menu.

### Ocarina

When you play the ocarina, the controls change. They are the same as on the N64
controller:

| Note | Control |
|---|---|
| A (D4) | A button |
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

## Mods

The game loads mods for Ship of Harkinian: texture packs, model packs, and text
packs. A mod is a file with the extension `.o2r` or `.otr`.

> [!IMPORTANT]
> This project does not contain mods and does not give mods. Get each mod from
> its author. Most mods change the assets of the game, thus each player gets
> them for the player's own copy.

### Install a mod

1. Download the mod from its author. If the mod is in a ZIP file, extract it.
2. Connect the headset to your computer, and open SideQuest.
3. In the SideQuest file manager, open
   `Android/data/org.oot.vr/files/mods/` on the headset.
4. Copy the `.o2r` or `.otr` files into that folder. You can put each mod in its
   own subfolder.
5. Start the game. The game turns on new mods automatically.

If you use `adb`, copy the mod to the `Download` folder first, and then move it.
A direct copy of a large file into the folder of the game can fail:

```
adb push <mod-folder> /sdcard/Download/
adb shell mv /sdcard/Download/<mod-folder> /sdcard/Android/data/org.oot.vr/files/mods/
```

### Manage the mods

Open the SoH menu (Y button), and go to **Settings** > **Mod Menu**.

- **Enable Mods** turns all the mods on or off.
- The list sets the order in which the game loads the mods. When two mods
  change the same texture, the mod that loads last is visible.

### Performance and memory

The Quest has less memory and a slower processor than a PC. Large texture packs
can make the game slow or close it.

- Keep **VR Settings** > **Performance** > **Headset Refresh Rate** at 72 Hz.
  72 Hz gives each frame more time than 90 Hz.
- Do not use two complete texture packs together. On a Quest 3S, OoT Reloaded
  together with the complete Djipi's 3DS Experience pack used more than 4 GB of
  memory. The frame rate fell to 17 FPS in Hyrule Field.
- If the game is slow after you add a mod, remove the mod and test again.

We tested these mods on a Quest 3S at 72 Hz:

| Mod | Result |
|---|---|
| [OoT Reloaded](https://github.com/GhostlyDark/OoT-Reloaded) (HD textures) | Works. About 860 MB of memory. |
| Djipi's 3DS Experience, only the background files 26, 27, 32, and 33 | Works with OoT Reloaded. The pre-rendered rooms become 3D. |
| Djipi's 3DS Experience, complete | Too slow with OoT Reloaded. |

### If a mod does not load

- Make sure that the file is in `Android/data/org.oot.vr/files/mods/` and has
  the extension `.o2r` or `.otr`.
- The game cannot read a folder that `adb` made with wrong permissions. The game
  then ignores that folder. To give the game access, run:

  ```
  adb shell chmod -R a+rwX /sdcard/Android/data/org.oot.vr/files/mods
  ```

- A text pack can show some texts in English when a texture pack also has HD
  versions of those texts. For example, OoT Reloaded has English HD versions of
  the area names and of the pause menu buttons. The HD versions replace the
  text pack. The dialogs of the text pack still show.

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
