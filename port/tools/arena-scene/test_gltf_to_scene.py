#!/usr/bin/env python3
"""Host tests for gltf_to_scene.py. Run them from the repository root:

    python3 -m unittest discover -s port/tools/arena-scene
"""

import base64
import json
import os
import struct
import tempfile
import unittest
import xml.etree.ElementTree as ET

import gltf_to_scene as g
from pngwrite import write_png


def make_gltf(directory, meshes, extra_nodes=(), texture_size=(32, 32), light=True, extras=None):
    """meshes: a list of (node name, positions, uvs, indices, transform dict, use texture)."""
    write_png(os.path.join(directory, "tex.png"), texture_size[0], texture_size[1], lambda x, y: (200, 100, 50, 255))
    blob = bytearray()
    views, accessors, gltf_meshes, nodes = [], [], [], []

    def add(values, fmt, kind, comp_type, count):
        offset = len(blob)
        for v in values:
            blob.extend(struct.pack("<" + fmt, *(v if isinstance(v, tuple) else (v,))))
        while len(blob) % 4:
            blob.append(0)
        views.append({"buffer": 0, "byteOffset": offset, "byteLength": len(blob) - offset})
        accessors.append({"bufferView": len(views) - 1, "componentType": comp_type, "count": count, "type": kind})
        return len(accessors) - 1

    for name, positions, uvs, indices, transform, textured in meshes:
        attrs = {"POSITION": add(positions, "fff", "VEC3", 5126, len(positions))}
        if uvs:
            attrs["TEXCOORD_0"] = add(uvs, "ff", "VEC2", 5126, len(uvs))
        prim = {"attributes": attrs, "indices": add(indices, "H", "SCALAR", 5123, len(indices)),
                "material": 0 if textured else 1}
        gltf_meshes.append({"primitives": [prim]})
        node = {"name": name, "mesh": len(gltf_meshes) - 1}
        node.update(transform)
        nodes.append(node)
    nodes.extend(extra_nodes)
    if light:
        nodes.append({"name": "sun", "rotation": [-0.7071068, 0.0, 0.0, 0.7071068],
                      "extensions": {"KHR_lights_punctual": {"light": 0}}})
    doc = {
        "asset": {"version": "2.0"},
        "scene": 0,
        "scenes": [{"nodes": list(range(len(nodes))), "extras": extras or {}}],
        "nodes": nodes,
        "meshes": gltf_meshes,
        "materials": [
            {"name": "stone", "pbrMetallicRoughness": {"baseColorTexture": {"index": 0}}},
            {"name": "plain", "pbrMetallicRoughness": {"baseColorFactor": [0.5, 0.5, 0.5, 1.0]}},
        ],
        "textures": [{"source": 0}],
        "images": [{"uri": "tex.png"}],
        "buffers": [{"byteLength": len(blob),
                     "uri": "data:application/octet-stream;base64," + base64.b64encode(bytes(blob)).decode()}],
        "bufferViews": views,
        "accessors": accessors,
        "extensions": {"KHR_lights_punctual": {"lights": [{"type": "directional", "intensity": 1.0}]}},
    }
    path = os.path.join(directory, "scene.gltf")
    with open(path, "w") as f:
        json.dump(doc, f)
    return path


# A floor of 2 x 2 m at y = 0. Its front faces up (counter-clockwise from above).
FLOOR = ("floor", [(-1, 0, -1), (-1, 0, 1), (1, 0, 1), (1, 0, -1)], [(0, 0), (0, 1), (1, 1), (1, 0)],
         [0, 1, 2, 0, 2, 3], {}, True)


def grid(n, size):
    """A floor of n x n quads, size meters wide, with UVs that repeat one time for each meter."""
    positions, uvs, indices = [], [], []
    for z in range(n + 1):
        for x in range(n + 1):
            px, pz = -size / 2 + size * x / n, -size / 2 + size * z / n
            positions.append((px, 0.0, pz))
            uvs.append((px + size / 2, pz + size / 2))
    for z in range(n):
        for x in range(n):
            a = z * (n + 1) + x
            indices += [a, a + n + 1, a + n + 2, a, a + n + 2, a + 1]
    return positions, uvs, indices


class CollisionPolygonTests(unittest.TestCase):
    def test_floor_normal_points_up_and_dist_is_minus_height(self):
        normal, dist = g.collision_polygon((0, 50, 0), (0, 50, 40), (40, 50, 40))
        self.assertEqual(normal, (0, 0x7FFF, 0))
        self.assertEqual(dist, -50)

    def test_wall_normal_points_to_its_front(self):
        # A wall in the plane z = -100, with its front to +Z.
        normal, dist = g.collision_polygon((0, 0, -100), (40, 0, -100), (40, 40, -100))
        self.assertEqual(normal, (0, 0, 0x7FFF))
        self.assertEqual(dist, 100)

    def test_degenerate_triangle_has_no_polygon(self):
        self.assertIsNone(g.collision_polygon((0, 0, 0), (1, 1, 1), (2, 2, 2)))


class BatchTests(unittest.TestCase):
    def test_batches_never_load_more_than_32_vertices(self):
        tris = [[(i, 0), (i, 1), (i, 2)] for i in range(40)]
        loads = list(g.batches(tris))
        self.assertEqual(sum(len(t) for _, t in loads), 40)
        for vertices, idx in loads:
            self.assertLessEqual(len(vertices), g.MAX_VERTICES_PER_LOAD)
            for tri in idx:
                self.assertTrue(all(0 <= i < len(vertices) for i in tri))

    def test_batches_share_equal_vertices(self):
        tris = [["a", "b", "c"], ["a", "c", "d"]]
        (vertices, idx), = list(g.batches(tris))
        self.assertEqual(vertices, ["a", "b", "c", "d"])
        self.assertEqual(idx, [(0, 1, 2), (0, 2, 3)])


class TexCoordTests(unittest.TestCase):
    def test_large_uvs_move_by_whole_repeats(self):
        mat = g.Material.__new__(g.Material)
        mat.texture_path, mat.width, mat.height = "x.png", 32, 32
        tri = g.Triangle([(0, 0, 0)] * 3, [(100.0, 50.5), (101.0, 50.5), (101.0, 51.5)], [(1, 1, 1, 1)] * 3, mat)
        self.assertEqual(g.tex_coords(tri), [(0, 512), (1024, 512), (1024, 1536)])


class ConversionTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = self.tmp.name

    def tearDown(self):
        self.tmp.cleanup()

    def convert(self, path):
        scene = g.load_scene(path)
        files, textures = g.build_files(scene, "test_scene", "scenes/shared/test_scene")
        return scene, files, textures

    def parse(self, files, name):
        return ET.fromstring(files[name].decode("utf-8"))

    def test_units_per_meter_and_node_transform(self):
        moved = FLOOR[:4] + ({"translation": [0.0, 1.0, 0.0], "scale": [2.0, 1.0, 2.0]}, True)
        scene, _, _ = self.convert(make_gltf(self.dir, [moved]))
        xs = sorted({p[0] for t in scene.draw for p in t.positions})
        ys = {p[1] for t in scene.draw for p in t.positions}
        self.assertEqual(xs, [-80.0, 80.0])
        self.assertEqual(ys, {40.0})

    def test_mirrored_node_keeps_the_floor_facing_up(self):
        mirrored = FLOOR[:4] + ({"scale": [-1.0, 1.0, 1.0]}, True)
        _, files, _ = self.convert(make_gltf(self.dir, [mirrored]))
        for poly in self.parse(files, "test_scene_col").iter("Polygon"):
            self.assertEqual(int(poly.get("NormalY")), 0x7FFF)

    def test_every_referenced_resource_exists(self):
        _, files, textures = self.convert(make_gltf(self.dir, [FLOOR]))
        resources = {"scenes/shared/test_scene/" + n for n in files}
        resources |= {"scenes/shared/test_scene/" + n.split(".")[0] for n in textures}
        for name, data in files.items():
            for node in ET.fromstring(data.decode("utf-8")).iter():
                for key in ("Path", "FileName", "MeshOpa"):
                    if node.get(key):
                        self.assertIn(node.get(key), resources, "%s in %s" % (key, name))

    def test_root_elements_match_the_resource_types_of_the_game(self):
        _, files, _ = self.convert(make_gltf(self.dir, [FLOOR]))
        self.assertEqual(self.parse(files, "test_scene").tag, "Room")
        self.assertEqual(self.parse(files, "test_scene_room_0").tag, "Room")
        self.assertEqual(self.parse(files, "test_scene_col").tag, "CollisionHeader")
        self.assertEqual(self.parse(files, "test_scene_room_0_dl").tag, "DisplayList")
        self.assertEqual(self.parse(files, "test_scene_room_0_vtx_0").tag, "Vertex")
        for name in files:
            self.assertEqual(self.parse(files, name).get("Version"), "0")

    def test_scene_commands_have_no_actor_list(self):
        _, files, _ = self.convert(make_gltf(self.dir, [FLOOR]))
        for name in ("test_scene", "test_scene_room_0"):
            commands = [c.tag for c in self.parse(files, name)]
            self.assertNotIn("SetActorList", commands)
            self.assertEqual(commands[-1], "EndMarker")
        scene_commands = [c.tag for c in self.parse(files, "test_scene")]
        for needed in ("SetRoomList", "SetCollisionHeader", "SetEntranceList", "SetStartPositionList",
                       "SetLightingSettings", "SetSkyboxSettings"):
            self.assertIn(needed, scene_commands)

    def test_vertex_loads_match_their_buffers(self):
        positions, uvs, indices = grid(8, 16)
        _, files, _ = self.convert(make_gltf(self.dir, [("floor", positions, uvs, indices, {}, True)]))
        dl = self.parse(files, "test_scene_room_0_dl")
        count = None
        tris = 0
        for cmd in dl:
            if cmd.tag == "LoadVertices":
                vtx_name = cmd.get("Path").rsplit("/", 1)[1]
                count = int(cmd.get("Count"))
                self.assertLessEqual(count, g.MAX_VERTICES_PER_LOAD)
                self.assertEqual(len(self.parse(files, vtx_name)), count)
            elif cmd.tag in ("Triangle1", "Triangles2"):
                keys = [k for k in cmd.keys() if k.startswith("V")]
                for k in keys:
                    self.assertLess(int(cmd.get(k)), count)
                tris += len(keys) // 3
        self.assertEqual(tris, 128)

    def test_texture_coordinates_stay_in_range_on_a_large_floor(self):
        positions, uvs, indices = grid(20, 400)
        _, files, _ = self.convert(make_gltf(self.dir, [("floor", positions, uvs, indices, {}, True)]))
        for name in files:
            if "_vtx_" in name:
                for vtx in self.parse(files, name):
                    self.assertTrue(-32768 <= int(vtx.get("S")) <= 32767)
                    self.assertTrue(-32768 <= int(vtx.get("T")) <= 32767)

    def test_collision_only_and_draw_only_nodes(self):
        wall = ("col_wall", [(-1, 0, -1), (1, 0, -1), (1, 2, -1)], None, [0, 1, 2], {}, False)
        deco = ("vis_deco", [(0, 0, 0), (0, 1, 0), (1, 1, 0)], None, [0, 1, 2], {}, False)
        scene, _, _ = self.convert(make_gltf(self.dir, [FLOOR, wall, deco]))
        self.assertEqual(len(scene.draw), 3)
        self.assertEqual(len(scene.collide), 3)

    def test_spawn_node_gives_the_start_position_and_yaw(self):
        # A rotation of 180 degrees about Y: the node looks along -Z.
        spawn = {"name": "spawn", "translation": [0.0, 0.0, 2.0], "rotation": [0.0, 1.0, 0.0, 0.0]}
        _, files, _ = self.convert(make_gltf(self.dir, [FLOOR], extra_nodes=[spawn]))
        entry = next(self.parse(files, "test_scene").iter("StartPositionEntry"))
        self.assertEqual((entry.get("PosX"), entry.get("PosY"), entry.get("PosZ")), ("0", "0", "80"))
        self.assertEqual(int(entry.get("RotY")), -0x8000)

    def test_light_points_to_the_sun(self):
        # The sun node turns -90 degrees about X: its light goes down, so the direction to the light is up.
        _, files, _ = self.convert(make_gltf(self.dir, [FLOOR]))
        light = next(self.parse(files, "test_scene").iter("LightingSetting"))
        self.assertEqual((light.get("Light1DirX"), light.get("Light1DirY"), light.get("Light1DirZ")),
                         ("0", "127", "0"))

    def test_scene_extras_set_the_ambient_light_and_the_fog(self):
        extras = {"ambient": [0.2, 0.2, 0.2], "fogColor": [0.1, 0.2, 0.3], "fogNear": 900, "fogFar": 5000}
        _, files, _ = self.convert(make_gltf(self.dir, [FLOOR], extras=extras))
        light = next(self.parse(files, "test_scene").iter("LightingSetting"))
        self.assertEqual(light.get("AmbientColorR"), "51")
        self.assertEqual((light.get("FogColorR"), light.get("FogColorG"), light.get("FogColorB")),
                         ("26", "51", "76"))
        self.assertEqual((light.get("FogNear"), light.get("FogFar")), ("900", "5000"))

    def test_texture_file_name_tells_the_format_to_the_build(self):
        _, _, textures = self.convert(make_gltf(self.dir, [FLOOR]))
        self.assertEqual(list(textures), ["test_scene_tex_stone.rgb5a1.png"])

    def test_texture_too_large_for_tmem_is_an_error(self):
        with self.assertRaises(g.SceneError):
            self.convert(make_gltf(self.dir, [FLOOR], texture_size=(64, 64)))

    def test_texture_size_must_be_a_power_of_two(self):
        with self.assertRaises(g.SceneError):
            self.convert(make_gltf(self.dir, [FLOOR], texture_size=(24, 32)))

    def test_position_out_of_range_is_an_error(self):
        far = FLOOR[:4] + ({"translation": [900.0, 0.0, 0.0]}, True)
        with self.assertRaises(g.SceneError):
            self.convert(make_gltf(self.dir, [far]))


class TestSceneTests(unittest.TestCase):
    """The test scene of make_test_scene.py: a floor, four walls that face the center, and one light."""

    def test_test_scene_converts(self):
        import make_test_scene
        with tempfile.TemporaryDirectory() as tmp:
            scene = g.load_scene(make_test_scene.make(tmp))
            files, textures = g.build_files(scene, "arena_scene", "scenes/shared/arena_scene")
        col = ET.fromstring(files["arena_scene_col"].decode("utf-8"))
        normals = {(int(p.get("NormalX")), int(p.get("NormalY")), int(p.get("NormalZ"))) for p in col.iter("Polygon")}
        self.assertEqual(normals, {(0, 0x7FFF, 0), (0, 0, 0x7FFF), (0, 0, -0x7FFF), (0x7FFF, 0, 0), (-0x7FFF, 0, 0)})
        self.assertEqual(len(textures), 2)
        entry = next(ET.fromstring(files["arena_scene"].decode("utf-8")).iter("StartPositionEntry"))
        self.assertEqual(entry.get("PosZ"), "360")
        light = next(ET.fromstring(files["arena_scene"].decode("utf-8")).iter("LightingSetting"))
        self.assertGreater(int(light.get("Light1DirY")), 0)


if __name__ == "__main__":
    unittest.main()
