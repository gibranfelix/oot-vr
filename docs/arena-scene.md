# A new scene for ARENA

This document gives the result of the research in
[#168](https://github.com/gibranfelix/oot-vr/issues/168).

**Answer: yes, with limits.** The game can load a scene from new data in
`soh.o2r`. The scene does not need data from `oot.o2r`. The limits are in
[Limits](#limits).

## Summary of the facts

| Question | Answer | Status |
|---|---|---|
| Can the `soh.o2r` build make a scene? | Yes. The build adds each file in `port/soh/assets/custom/` to `soh.o2r`. The game has XML loaders for the scene, the room, the collision, and the display lists. | Examined in the code |
| Which data does a scene need? | A scene file, one room file or more, a collision header, display lists, vertices, and textures. Read [Data of a scene](#data-of-a-scene). | Examined in the code |
| Which tool makes the geometry? | Blender. A new script must convert the Blender export to the XML files. Read [Tools](#tools). | The script is not written |
| Does the scene need a scene ID and an entrance? | Yes. The tables have fixed sizes. Use a slot of a debug scene. Read [Scene ID and entrance](#scene-id-and-entrance). | Examined in the code |
| Does the scene work in VR? | Probably. The scale and the lights are known. The performance is not tested. | Not tested on the headset |

## How the game loads a scene

1. `OTRPlay_SpawnScene` (`port/soh/soh/z_play_otr.cpp`) reads
   `gSceneTable[sceneId].sceneFile.fileName`.
2. It loads the resource `scenes/shared/<name>/<name>`. Only the dungeons with
   a Master Quest version use `scenes/nonmq/` or `scenes/mq/`.
3. If the resource does not load, the game loads Dodongo's Cavern.
4. The scene commands load the rooms with the paths in the command
   `SetRoomList`.

The resource manager does not care which archive has the file. Thus, a scene
in `soh.o2r` loads as a scene in `oot.o2r` loads.

### The order of the archives

The game adds `soh.o2r` first, then `oot.o2r` (`OTRGlobals.cpp`). For each
file path, the archive that the game adds last is the archive that the game
uses (`ArchiveManager::AddArchive` in `libultraship`). Thus, a file in
`oot.o2r` replaces a file with the same path in `soh.o2r`.

The arena must use a path that is not in `oot.o2r`. Read
[Scene ID and entrance](#scene-id-and-entrance).

## Data of a scene

The game reads the scene file and the room files with the same loader. In XML,
the root element of the two files is `<Room>`. Each child element is one scene
command. The XML loaders are in `port/soh/soh/resource/importer/`.

| Data | Scene command or resource | XML loader |
|---|---|---|
| Room list | `SetRoomList`, with one `Path` for each room | `SetRoomListFactory.cpp` |
| Collision | `SetCollisionHeader`, and a `<CollisionHeader>` resource | `CollisionHeaderFactory.cpp` |
| Geometry | `SetMesh` in the room, with `MeshOpa` and `MeshXlu` paths | `SetMeshFactory.cpp` |
| Display lists | `<DisplayList>` resources | `libultraship` (`Fast::ResourceFactoryXMLDisplayListV0`) |
| Vertices | `<Vertex>` resources | `libultraship` (`Fast::ResourceFactoryXMLVertexV0`) |
| Textures | PNG files. The build converts them. | `OTRExporter/OTRExporter/Main.cpp` |
| Entrance | `SetStartPositionList` and `SetEntranceList` | `SetStartPositionListFactory.cpp`, `SetEntranceListFactory.cpp` |
| Lights | `SetLightingSettings` (ambient, two directional lights, fog) | `SetLightingSettingsFactory.cpp` |
| Skybox | `SetSkyboxSettings` (`SkyboxId`, `Indoors`) | `SetSkyboxSettingsFactory.cpp` |
| Objects | `SetSpecialObjects` and `SetObjectList` | `SetSpecialObjectsFactory.cpp`, `SetObjectListFactory.cpp` |
| Sound | `SetSoundSettings` (the music sequence) | `SetSoundSettingsFactory.cpp` |
| Room behavior | `SetRoomBehavior` | `SetRoomBehaviorFactory.cpp` |

`soh.o2r` already has XML display lists and XML vertices. An example is
`port/soh/assets/custom/objects/gameplay_field_keep/gFieldBushBossKeyDL`.
`soh.o2r` also has a room for the debug scene `syotes_scene`, in
`port/soh/assets/custom/scenes/shared/`.

### How the build adds the files

`GenerateSohOtr` runs `OTRExporter` with `--norom`. `OTRExporter` adds each
file in `port/soh/assets/custom/` to `soh.o2r`:

- A file `<name>.<format>.png` becomes the texture `<name>`. The formats are
  `rgba32`, `rgb5a1`, `i4`, `i8`, `ia4`, `ia8`, `ia16`, `ci4`, and `ci8`.
- Each other file goes into the archive with no change. The path in the
  archive is the path below `custom/`.

Thus, the XML files of the arena go in
`port/soh/assets/custom/scenes/shared/<arena>/`. The build needs no change.

### The data from `oot.o2r`

The scene geometry, the collision, and the textures can all be new. Some data
that the scene uses comes from `oot.o2r`:

- Link, the enemies, the shop, and `gameplay_keep`.
- The object that `SetSpecialObjects` selects: `gameplay_field_keep` or
  `gameplay_dangeon_keep`.
- The skybox textures. To use no skybox, set `Indoors="1"` and a `SkyboxId`
  of 0.

This data is not part of the scene. The ARENA scenes of today use the same
data.

## Tools

The XML formats are simple, and the loaders show each attribute. We did not
find a tool that exports a full scene in these XML formats.

| Tool | What it makes | Status |
|---|---|---|
| Fast64 (Blender), the fork of the Ship of Harkinian community | Display lists and skeletons in XML for Ship of Harkinian | Not tested. We did not find documentation for the export of a scene or a collision header. |
| Fast64 (Blender), the main version | A scene in C for the decompilation | The game cannot read C. A converter is necessary. |
| A new script in this repository | All the XML files of the scene | Not written |

We recommend a new script, for example `tools/arena-scene/`:

1. Make the arena in Blender. Export it as glTF.
2. The script reads the glTF file. It writes the room display lists, the
   vertices, the collision header, the scene file, and the room file.
3. The script copies the textures as `<name>.rgba32.png` files.

The script must obey these rules of the formats:

- The positions are signed 16-bit integers. The arena must stay between
  -32768 and 32767 units on each axis.
- Each collision polygon has a normal and a distance. The script must
  calculate them: the normal is a unit vector times 32767.
- The collision header has the minimum bounds and the maximum bounds.
- A display list loads a maximum of 32 vertices at one time. The script must
  cut the mesh into groups of 32 vertices or fewer.

The Blender file and the PNG files must be new. Do not use geometry or textures
from the game.

## Scene ID and entrance

The game finds a scene with its scene ID. `gSceneTable` has `SCENE_ID_MAX`
entries, and the entrance table has a fixed size. Ship of Harkinian has no
function that adds a scene ID or an entrance. The mods of Ship of Harkinian
replace the files of a scene that is in the tables.

The tables have nine debug scenes, from `0x65` (`SCENE_TEST01`) to `0x6D`
(`SCENE_TESTROOM`). Each debug scene has entrances, for example
`ENTR_TESTROOM_0` (`0x024`). The XML files of the retail versions do not
extract the debug scenes. Only the debug ROMs have them.

We recommend this procedure:

1. Use one slot of a debug scene, for example `SCENE_TESTROOM` and
   `ENTR_TESTROOM_0`.
2. Give the arena its own path, for example `scenes/shared/arena_scene/`.
3. In `OTRPlay_SpawnScene`, load the path of the arena when ARENA uses this
   slot. Mark the change with `SOH [VR]`.

Step 2 is necessary. A debug ROM makes an `oot.o2r` with
`scenes/shared/testroom_scene/`, and that file replaces a file in `soh.o2r`.

## VR

### Scale

Adult Link has a height of 68 units (`docs/architecture.md`). Thus, one meter
is approximately 40 units in the world of adult Link. The VR layer calculates
the world scale from the eye height of Link. A new scene does not change it.

| Arena of today | Radius | Radius in meters |
|---|---|---|
| Phantom Ganon | Approximately 300 units | Approximately 7.5 m |
| Ganon (fire ring) | 784 units | Approximately 20 m |

### Lights

`SetLightingSettings` sets the ambient light, two directional lights, and the
fog. The Ganon arena of today is too dark, and ARENA adds an ambient light. A
new scene can have the correct light in its data.

### Enemies

ARENA spawns the enemies with `Actor_Spawn`. If the object of an enemy is not
in the object list, `Actor_Spawn` uses bank 0 (`z_actor.c`). The resources of
the enemy load by their paths. The two ARENA scenes of today use this
behavior. Thus, the object list of the new scene can be empty.

### Performance

We did not test the performance on the Quest 3S. A simple arena has fewer
triangles than most scenes of the game. Measure the frame time on the headset
with the first version of the scene.

## Limits

- **No scene export tool.** A new script must make the XML files.
- **One slot.** The arena uses the slot of a debug scene. The number of free
  slots is nine.
- **No test of a full XML scene.** `soh.o2r` has XML display lists and one
  binary room. We did not load a scene with XML in all of its files.
- **Data from `oot.o2r`.** Link, the enemies, and the special objects come from
  `oot.o2r`.
- **The pause screen.** The debug scenes have no map and no title card. Make
  sure that the pause screen does not show a map for the arena.

## Next step

Make a test scene before the design: a flat floor, four walls, and one light.
This test shows that the full chain works: Blender, the script, `soh.o2r`, and
the headset.
