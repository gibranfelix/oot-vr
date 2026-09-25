# Architecture

This document tells you where the VR layer connects to the game and to the
engine. Read it before you change VR code.

## The three parts

| Part | Directory | Contents |
|---|---|---|
| Game | `port/soh` | Ship of Harkinian 9.2.3 with the VR changes from `Shipwright-VR` |
| Engine | `port/libultraship` | The Fast3D renderer, the GLES backend, and the OpenXR session |
| Android wrapper | `port/Android` | The Gradle project, `MainActivity`, and the manifest for Horizon OS |

The game and the Android wrapper use SDL2. Upstream `libultraship` changed to
SDL3. Thus, we do not merge upstream changes. We copy each fix by hand. Read
[`adr/0002`](adr/0002-one-repository-with-the-port-inside.md).

## How to find VR code

All VR changes in upstream files have the comment marker `SOH [VR]`. To find
them, run:

```
git grep -n 'SOH \[VR\]' port
```

The VR code that is new has its own files:

| File | Contents |
|---|---|
| `port/libultraship/src/fast/vr_openxr.cpp` | The OpenXR session, the swapchains, and the eye poses |
| `port/libultraship/src/fast/vr_interface.cpp` | The C interface that the game calls (`VR_*` functions) |
| `port/libultraship/include/vr_interface.h` | The declarations of the `VR_*` functions, with comments |
| `port/libultraship/src/fast/vr_physics.cpp` | The collision mesh for the physical hands |
| `port/soh/soh/Enhancements/vr-combat/` | Sword swing, shield, and item selection |
| `port/soh/soh/SohGui/SohMenuVRSettings.cpp` | The VR settings menu |

## Render path

`libultraship` does not know about stereo. The renderer gets triangles that are
already in clip space. Thus, the VR layer must change the projection before the
renderer gets the triangles.

The connection point is `Interpreter::GfxSpMatrix` in
`port/libultraship/src/fast/interpreter.cpp`. When the game loads a perspective
projection, the VR layer replaces it with the view and projection of the
current eye. 2D targets keep the projection of the game. The 2D targets are the
HUD quad and the floating menu panel.

Each VR pass renders into an OpenXR swapchain. The frame buffers of the
`libultraship` backend make each eye pass a normal off-screen render.

`vr_openxr.cpp` has two graphics paths:

- `XR_USE_GRAPHICS_API_OPENGL_ES`: the Quest path. This path works.
- `XR_USE_GRAPHICS_API_D3D11`, under `ENABLE_DX11`: the Windows path. We
  changed this path but we did not compile it again. Think that it is broken.

## Camera and movement

- **First-person camera.** Vanilla Ocarina of Time has a first-person camera:
  `Camera_Subj3` in `port/soh/src/code/z_camera.c`. The game sends the position
  of the head of Link to the VR layer with `VR_SetCameraAnchor`. The VR layer
  adds the offset and the rotation of the headset.
- **Movement.** `Player_GetMovementSpeedAndYaw` in
  `port/soh/src/overlays/actors/ovl_player_actor/z_player.c` gives the speed and
  the direction for all locomotion actions. The VR layer changes the direction
  at this one point.
- **Cutscenes.** Cutscenes use a different sub-camera (`z_demo.c`). Thus, the
  code can find a cutscene with one check.
- **World scale.** Link has two heights: 68.0 for adult and 44.0 for child. The
  game sends the eye height of Link with `VR_SetLinkEyeHeight`. The VR layer
  compares it with the real eye height of the player and calculates the scale.

## Android wrapper

The `MainActivity` of `linkzenic/Shipwright-Android` has approximately 1500
lines. It has a ROM picker, an extraction dialog, and a touch overlay. The VR
build does not use these parts. The player makes `oot.o2r` on a PC, and the
input comes from OpenXR. Our `MainActivity` has approximately 110 lines.

The wrapper keeps the extractor configuration files (300 KB). `RunExtract()`
needs this directory at each start, also when `oot.o2r` is present. Without the
directory, the game waits for a click on a dialog. In VR, you cannot click that
dialog.

## Debug on the headset

The headset has no console and no debugger. Use these tools:

| Tool | Command or location |
|---|---|
| Game log | `adb logcat -s soh:V`. VR messages start with `[VR]`. |
| Log files | `/sdcard/Android/data/org.oot.vr/files/logs/` |
| Screenshot of the left eye | `adb exec-out screencap -p > s.png`. Not tested on the device. |
| Headset recordings | `/sdcard/Oculus/VideoShots/`. Get them with `adb pull`. |
| Save file with all items | In `shipofharkinian.json`, set `gDeveloperTools.DebugEnabled=1` and `DebugSaveFileMode=2`. A new save in slot 1 then has all items. Developer mode also gives a scene selector. |

Horizon OS does not start the app when nobody wears the headset. You cannot
change this with `adb`. Put on the headset for each test.
