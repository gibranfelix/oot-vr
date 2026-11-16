#!/usr/bin/env python3
"""Make the glTF file of the ARENA test scene (issue #170): a floor, four walls, and one light.

The geometry and the textures are new. The script makes them from numbers, so the repository contains no game assets.
Open the glTF file in Blender to change the scene. Then convert it with gltf_to_scene.py.
"""

import json
import math
import os
import random
import sys

from gltfwrite import GltfBuffer, write_png

# The size of the room, in meters. Adult Link is about 1.7 m.
HALF_SIZE = 12.0
WALL_HEIGHT = 4.0
# One quad of the floor and the walls, in meters.
TILE = 4.0
# One repeat of a texture, in meters.
TEXTURE_REPEAT = 2.0
SPAWN_Z = 9.0
TILES_PER_SIDE = int(2 * HALF_SIZE / TILE)
# The direction to the sun: above the room, from the south-east. The walls get different light.
SUN_DIRECTION = (0.5, 1.0, 0.7)


def rotation_from_z(d):
    """The quaternion (x, y, z, w) that turns +Z to the direction d."""
    length = math.sqrt(sum(c * c for c in d))
    d = [c / length for c in d]
    axis = (-d[1], d[0], 0.0)  # cross((0, 0, 1), d)
    axis_length = math.sqrt(axis[0] ** 2 + axis[1] ** 2)
    if axis_length == 0.0:
        return [0.0, 0.0, 0.0, 1.0] if d[2] > 0 else [1.0, 0.0, 0.0, 0.0]
    angle = math.acos(max(-1.0, min(1.0, d[2])))
    s = math.sin(angle / 2) / axis_length
    return [axis[0] * s, axis[1] * s, 0.0, math.cos(angle / 2)]


def floor_texture(path):
    rng = random.Random(170)
    noise = [[rng.randint(-10, 10) for _ in range(32)] for _ in range(32)]

    def pixel(x, y):
        # Four stone slabs with dark joints.
        if x % 16 in (0, 15) or y % 16 in (0, 15):
            v = 70 + noise[y][x] // 2
            return (v, v, v + 5, 255)
        slab = 120 + ((x // 16 + y // 16) % 2) * 15 + noise[y][x]
        return (slab, slab - 4, slab - 10, 255)

    write_png(path, 32, 32, pixel)


def wall_texture(path):
    rng = random.Random(171)
    noise = [[rng.randint(-12, 12) for _ in range(32)] for _ in range(32)]

    def pixel(x, y):
        # Bricks of 16 x 8 pixels. Each second row moves by a half brick.
        row = y // 8
        bx = (x + (8 if row % 2 else 0)) % 16
        if y % 8 == 7 or bx == 15:
            v = 60 + noise[y][x] // 2
            return (v, v, v, 255)
        r = 140 + noise[y][x]
        return (r, r - 30, r - 55, 255)

    write_png(path, 32, 32, pixel)


def quads_floor():
    positions, uvs, indices = [], [], []
    n = TILES_PER_SIDE
    for iz in range(n):
        for ix in range(n):
            x0, z0 = -HALF_SIZE + ix * TILE, -HALF_SIZE + iz * TILE
            x1, z1 = x0 + TILE, z0 + TILE
            base = len(positions)
            # Counter-clockwise from above, so the front faces up.
            positions += [(x0, 0.0, z0), (x0, 0.0, z1), (x1, 0.0, z1), (x1, 0.0, z0)]
            uvs += [(x / TEXTURE_REPEAT, z / TEXTURE_REPEAT) for x, _, z in positions[base:]]
            indices += [base, base + 1, base + 2, base, base + 2, base + 3]
    return positions, uvs, indices


def quads_walls():
    positions, uvs, indices = [], [], []
    n = TILES_PER_SIDE
    h = HALF_SIZE
    # Each wall: a point on the wall, the direction along the wall, and the front faces the center.
    walls = [
        ((-h, 0.0, -h), (1.0, 0.0, 0.0)),  # North wall (z = -h), front to +Z.
        ((h, 0.0, h), (-1.0, 0.0, 0.0)),   # South wall (z = +h), front to -Z.
        ((-h, 0.0, h), (0.0, 0.0, -1.0)),  # West wall (x = -h), front to +X.
        ((h, 0.0, -h), (0.0, 0.0, 1.0)),   # East wall (x = +h), front to -X.
    ]
    for start, along in walls:
        for i in range(n):
            a = (start[0] + along[0] * i * TILE, 0.0, start[2] + along[2] * i * TILE)
            b = (a[0] + along[0] * TILE, 0.0, a[2] + along[2] * TILE)
            base = len(positions)
            positions += [a, b, (b[0], WALL_HEIGHT, b[2]), (a[0], WALL_HEIGHT, a[2])]
            u0 = i * TILE / TEXTURE_REPEAT
            u1 = u0 + TILE / TEXTURE_REPEAT
            v = WALL_HEIGHT / TEXTURE_REPEAT
            uvs += [(u0, v), (u1, v), (u1, 0.0), (u0, 0.0)]
            # Counter-clockwise when you look at the wall from the center.
            indices += [base, base + 1, base + 2, base, base + 2, base + 3]
    return positions, uvs, indices


def make(out_dir):
    os.makedirs(out_dir, exist_ok=True)
    floor_texture(os.path.join(out_dir, "floor.png"))
    wall_texture(os.path.join(out_dir, "wall.png"))

    buf = GltfBuffer()
    meshes = []
    for material, (positions, uvs, indices) in enumerate((quads_floor(), quads_walls())):
        prim = {
            "attributes": {
                "POSITION": buf.add(positions, "fff", "VEC3", 5126, 34962),
                "TEXCOORD_0": buf.add(uvs, "ff", "VEC2", 5126, 34962),
            },
            "indices": buf.add(indices, "H", "SCALAR", 5123, 34963),
            "material": material,
        }
        meshes.append({"name": ("floor", "walls")[material], "primitives": [prim]})

    doc = {
        "asset": {"version": "2.0", "generator": "oot-vr make_test_scene.py"},
        "extensionsUsed": ["KHR_lights_punctual"],
        "scene": 0,
        "scenes": [{
            "name": "arena_scene",
            "nodes": [0, 1, 2, 3],
            "extras": {"ambient": [0.45, 0.45, 0.5], "fogColor": [0.05, 0.05, 0.08], "fogNear": 996,
                       "fogFar": 12800},
        }],
        "nodes": [
            {"name": "floor", "mesh": 0},
            {"name": "walls", "mesh": 1},
            # The light of a directional light goes along -Z of its node.
            {"name": "sun", "rotation": rotation_from_z(SUN_DIRECTION),
             "extensions": {"KHR_lights_punctual": {"light": 0}}},
            # Link looks to the center (-Z).
            {"name": "spawn", "translation": [0.0, 0.0, SPAWN_Z], "rotation": [0.0, 1.0, 0.0, 0.0]},
        ],
        "meshes": meshes,
        "materials": [
            {"name": "floor", "pbrMetallicRoughness": {"baseColorTexture": {"index": 0}, "metallicFactor": 0.0}},
            {"name": "wall", "pbrMetallicRoughness": {"baseColorTexture": {"index": 1}, "metallicFactor": 0.0}},
        ],
        "textures": [{"source": 0}, {"source": 1}],
        "images": [{"uri": "floor.png"}, {"uri": "wall.png"}],
        "extensions": {"KHR_lights_punctual": {"lights": [{"name": "sun", "type": "directional",
                                                           "color": [1.0, 0.95, 0.85], "intensity": 0.6}]}},
    }
    buf.fill(doc)
    path = os.path.join(out_dir, "arena_scene.gltf")
    with open(path, "w") as f:
        json.dump(doc, f, indent=1)
        f.write("\n")
    return path


if __name__ == "__main__":
    print(make(sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), "test-scene")))
