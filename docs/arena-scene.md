# A new ARENA scene

**Answer: yes, with limits.** The game can load a scene from new data in
`soh.o2r`. The scene geometry, the collision, and the textures do not need data
from `oot.o2r`. This document gives the result of
[#168](https://github.com/gibranfelix/oot-vr/issues/168).

| Question of #168 | Answer | Section |
|---|---|---|
| Can the `soh.o2r` build make a scene? | Yes, with no change to the build. | [How the build adds the files](#how-the-build-adds-the-files) |
| Which data does a scene need? | A scene file, room files, a collision header, display lists, vertices, and textures. | [Data of a scene](#data-of-a-scene) |
| Which tool makes the geometry? | Blender, and a new script that writes the XML files. We did not write the script. | [Tools](#tools) |
| Does the scene need a scene ID and an entrance? | Yes. Use the scene ID of a debug scene. | [Scene ID and entrance](#scene-id-and-entrance) |
| Does the scene work in VR? | The scale and the lights are not a problem. We did not test the performance on the headset. | [VR](#vr) |

## How the game loads a scene

1. `OTRPlay_SpawnScene` (`port/soh/soh/z_play_otr.cpp`) reads
   `gSceneTable[sceneId].sceneFile.fileName`.
2. It loads the resource `scenes/shared/<name>/<name>`. Only the dungeons with
   a Master Quest version use `scenes/nonmq/` or `scenes/mq/`.
3. If the resource does not load, the game loads Dodongo's Cavern.
4. The scene commands load the rooms with the paths in the command
   `SetRoomList`.

The resource manager does not check which archive has the resource. Thus, a
scene in `soh.o2r` loads as a scene in `oot.o2r` loads.

### The order of the archives

The game adds `soh.o2r` first, then `oot.o2r` (`OTRGlobals.cpp`). For each
path, the game uses the archive that it added last
(`ArchiveManager::AddArchive` in `libultraship`). Thus, a resource in
`oot.o2r` replaces a resource with the same path in `soh.o2r`.

## Data of a scene

The scene file and the room files have the same XML format. The root element
is `<Room>`, and each child element is one scene command. The XML loaders are
in `port/soh/soh/resource/importer/`.

| Data | Scene command or resource | XML loader |
|---|---|---|
| Room list | `SetRoomList`, with one `Path` for each room | `SetRoomListFactory.cpp` |
| Collision | `SetCollisionHeader`, and a `<CollisionHeader>` resource | `CollisionHeaderFactory.cpp` |
| Geometry | `SetMesh` in the room, with `MeshOpa` and `MeshXlu` paths | `SetMeshFactory.cpp` |
| Display lists | `<DisplayList>` resources | `libultraship` (`Fast::ResourceFactoryXMLDisplayListV0`) |
| Vertices | `<Vertex>` resources | `libultraship` (`Fast::ResourceFactoryXMLVertexV0`) |
| Textures | PNG files | `OTRExporter/OTRExporter/Main.cpp` converts them |
| Entrance | `SetStartPositionList` and `SetEntranceList` | `SetStartPositionListFactory.cpp`, `SetEntranceListFactory.cpp` |
| Lights | `SetLightingSettings`: ambient light, two directional lights, fog | `SetLightingSettingsFactory.cpp` |
| Skybox | `SetSkyboxSettings`: `SkyboxId`, `Indoors` | `SetSkyboxSettingsFactory.cpp` |
| Environment | `SetTimeSettings`, `SetWind`, `SetEchoSettings`, `SetCameraSettings` | The factory with the same name |
| Objects | `SetSpecialObjects` and `SetObjectList` | `SetSpecialObjectsFactory.cpp`, `SetObjectListFactory.cpp` |
| Sound | `SetSoundSettings`: the music sequence | `SetSoundSettingsFactory.cpp` |
| Room behavior | `SetRoomBehavior` | `SetRoomBehaviorFactory.cpp` |

`soh.o2r` has XML display lists and XML vertices now. An example is
`port/soh/assets/custom/objects/gameplay_field_keep/gFieldBushBossKeyDL`.
`soh.o2r` also has a binary room for the debug scene `syotes_scene`. No
resource in `soh.o2r` is an XML scene or an XML collision header.

### How the build adds the files

`GenerateSohOtr` runs `OTRExporter` with `--norom`. `OTRExporter` adds each
file in `port/soh/assets/custom/` to `soh.o2r`:

- A file `<name>.<format>.png` becomes the texture `<name>`. The formats are
  `rgba32`, `rgb5a1`, `i4`, `i8`, `ia4`, `ia8`, `ia16`, `ci4`, and `ci8`.
- Each other file goes into the archive with no change. The path in the
  archive is the path below `custom/`.

Put the XML files of the ARENA scene in
`port/soh/assets/custom/scenes/shared/<name>/`.

### Data from `oot.o2r`

These items come from `oot.o2r`. They are not part of the scene geometry, the
collision, or the textures:

- Link, the enemies, the Business Scrubs, and `gameplay_keep`.
- The skybox textures. To use no skybox, set `Indoors="1"` and `SkyboxId="0"`.

`SetSpecialObjects` can load `gameplay_field_keep` or `gameplay_dangeon_keep`
from `oot.o2r`. A value of 0 loads no object (`z_scene_otr.cpp`).

## Tools

The loaders show each attribute of the XML formats. We did not find a tool
that exports a full scene in these formats.

| Tool | Output | Status |
|---|---|---|
| Fast64 for Blender, the fork of the Ship of Harkinian community | Display lists and skeletons in XML | We did not test it. We did not find documentation for a scene or a collision header. |
| Fast64 for Blender, the main version | A scene in C for the decompilation | The game cannot read C. A converter is necessary. |
| A new script in this repository, for example `tools/arena-scene/` | All the XML files of the scene | We did not write it. |

We recommend the new script. Use this procedure:

1. Make the ARENA scene in Blender. Use only new geometry and new textures.
2. Export the scene as glTF.
3. Run the script on the glTF file. The script writes the XML files and the
   PNG files.
4. Copy the files to `port/soh/assets/custom/scenes/shared/<name>/`.

The script must obey these rules of the formats:

- The positions are signed 16-bit integers. Each axis must stay between -32768
  and 32767 units.
- Each collision polygon has a normal and a distance. The normal is a unit
  vector times 32767.
- Each collision polygon has a `Type`. `Type` is an index into the
  `SurfaceType` list (`Data1`, `Data2`). The script must write this list.
- The collision header has the minimum bounds, the maximum bounds, and the
  camera data.
- `libultraship` accepts 64 vertices in one vertex load (`MAX_VERTICES`). The
  original microcode accepts 32. Groups of 32 vertices work with the two
  values.

## Scene ID and entrance

The game finds a scene with its scene ID. `gSceneTable` and the entrance table
have fixed sizes. Ship of Harkinian has no function that adds a scene ID or an
entrance. The mods of Ship of Harkinian replace the resources of a scene that
is in the tables.

The tables have nine debug scenes, from `0x65` (`SCENE_TEST01`) to `0x6D`
(`SCENE_TESTROOM`). Each debug scene has entrances. Only the XML files of the
debug versions extract the debug scenes. The XML files of the retail versions
do not extract them.

`SCENE_TESTROOM` is the best debug scene for ARENA:

- Its title card is `none`, and its draw configuration is `SDC_DEFAULT`
  (`scene_table.h`). Some other debug scenes have a title card or a special
  draw configuration.
- Its entrance is `ENTR_TESTROOM_0` (`0x024`).
- `sceneFlags` has 124 entries, and `0x6D` is in this range. The map code uses
  its default case for this scene ID.

Use this procedure:

1. Give the ARENA scene its own path, for example
   `scenes/shared/arena_scene/arena_scene`.
2. In `OTRPlay_SpawnScene`, load this path when ARENA uses `SCENE_TESTROOM`.
3. Mark the change with `SOH [VR]`.

Step 1 is necessary. An `oot.o2r` from a debug version has
`scenes/shared/testroom_scene/`. That `oot.o2r` replaces the resources of
`soh.o2r` with the same path.

## VR

### Scale

Adult Link has a height of 68 units (`docs/architecture.md`). Thus, one meter
is approximately 40 units in the world of adult Link. The VR layer calculates
the world scale from the eye height of Link. A new scene does not change it.

| ARENA scene | Radius | Radius in meters | Source |
|---|---|---|---|
| Phantom Ganon | Approximately 300 units | Approximately 7.5 m | `z_en_fhg.c`, comment in `Arena.cpp` |
| Ganon | 784 units, the fire ring | Approximately 20 m | `z_boss_ganon2.c`, comment in `Arena.cpp` |

### Lights

`SetLightingSettings` sets the ambient light, two directional lights, and the
fog. In the Ganon scene, `Arena.cpp` adds an ambient light because the fight
is dark. A new scene can have the correct light in its data.

### Enemies

ARENA spawns the enemies with `Actor_Spawn` after the scene loads. At that
time, `Actor_Spawn` uses bank 0 for an enemy that is not in the object list
(`z_actor.c`). The resources of the enemy load by their paths. Thus, the
object list of the ARENA scene can be empty.

This rule is not true while the scene loads. Do not put enemies in
`SetActorList`.

### Performance

We did not measure the performance on the headset. A simple ARENA scene has
fewer triangles than most scenes of the game. Measure the frame time on the
headset with the first version of the scene.

## Limits

- No tool exports the scene. A new script must make the XML files.
- We did not load a scene that is XML in all of its files.
- ARENA uses the scene ID of a debug scene. The game has no free scene IDs.

## Next step

Make a test scene before the design: a flat floor, four walls, and one light.
This test shows the full chain: Blender, the script, `soh.o2r`, and the
headset. [#170](https://github.com/gibranfelix/oot-vr/issues/170) has the
test scene and the design.
