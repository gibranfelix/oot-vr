# ARENA scene tools

These scripts make the ARENA scene in `soh.o2r` from a glTF file. They use
only the Python standard library.

| File | Purpose |
|---|---|
| `gltf_to_scene.py` | Converts a glTF file to the XML files and the PNG textures of a scene. |
| `make_test_scene.py` | Makes `test-scene/arena_scene.gltf`: a floor, four walls, and one light. |
| `test_gltf_to_scene.py` | The host tests of the converter. |
| `test-scene/` | The glTF file and the textures of the test scene. |

## Change the scene

1. Open `test-scene/arena_scene.gltf` in Blender. Change the scene.
2. Export the scene as glTF 2.0. Select "glTF Separate", so that the textures
   are PNG files. Select "Punctual Lights" and "Custom Properties".
3. Run the converter from the repository root:

   ```sh
   python3 port/tools/arena-scene/gltf_to_scene.py \
       port/tools/arena-scene/test-scene/arena_scene.gltf \
       --name arena_scene --custom-assets port/soh/assets/custom
   ```

4. Add the files with `git add -f`. The `.gitignore` of Ship of Harkinian
   ignores `*.png`.
5. Build the APK with `port/Android/build-apk.sh`. The build adds the files to
   `soh.o2r`.
6. If the size or the spawn point of the scene changes, change the entry of the
   test scene in `sArenaScenes` (`port/soh/soh/Enhancements/arena/Arena.cpp`).

Use only new geometry and new textures. Do not use data from the game.

## Rules for the glTF file

- One meter in Blender is 40 units in the game. Adult Link has a height of 68
  units.
- Each position must stay between -819 m and 819 m.
- Each triangle is drawn and collides. A node with a name that starts with
  `col_` only collides. A node with a name that starts with `vis_` is only
  drawn.
- A texture is a PNG file. Its width and its height are powers of two. Its
  width times its height is 2048 pixels or fewer, for example 32 x 64.
- A texture of 32 pixels can repeat fewer than 32 times across one triangle.
- One directional light lights Link and the enemies. The converter also adds
  it to the vertex colors of the scene. An intensity above 1 counts as 1.
- A node with the name `spawn` gives the start position and the direction of
  Link.
- The extras of the Blender scene can set `ambient` and `fogColor` (red,
  green, and blue from 0 to 1), `fogNear`, and `fogFar`.

## Tests

Run the tests from the repository root:

```sh
python3 -m unittest discover -s port/tools/arena-scene
```
