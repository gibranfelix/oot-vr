# Status

Last update: 2026-09-29.

## What works

- The full main story. We played from Kokiri Forest to Ganon's Tower on a
  Quest 3S.
- Stereo render, first-person camera, and head tracking.
- The physical sword and shield with the Touch controllers.
- Automatic world scale for child Link and adult Link.
- Menus and the pause screen on a floating panel.
- The wrist HUD. Hearts, magic, and rupees show on the off-hand wrist when you
  look at it. The item buttons show above the sword-hand controller. We tested
  it in the Twinrova fight.

## Known problems

| Problem | Issue |
|---|---|
| Houses, shops, and the Market show a gray mesh without texture | [#17](https://github.com/gibranfelix/oot-vr/issues/17) |
| Link shouts each time you swing the sword | [#19](https://github.com/gibranfelix/oot-vr/issues/19) |
| A gray rectangle shows during scene transitions | [#20](https://github.com/gibranfelix/oot-vr/issues/20) |
| A bomb shows as a pink sphere without texture | [#21](https://github.com/gibranfelix/oot-vr/issues/21) |
| The shield in your hand blocks the view | [#22](https://github.com/gibranfelix/oot-vr/issues/22) |
| A received item stays inside the camera | [#23](https://github.com/gibranfelix/oot-vr/issues/23) |
| The cutscene camera goes into the geometry (Rauru) | [#24](https://github.com/gibranfelix/oot-vr/issues/24) |
| The game starts two times at each launch | [#25](https://github.com/gibranfelix/oot-vr/issues/25) |
| When you put your head into a wall, you can see behind it | [#26](https://github.com/gibranfelix/oot-vr/issues/26) |

The first-start setup works on a Quest 3S: the panel asks for the ROM, the
headset makes `oot.o2r`, and the game starts in VR. We tested it with the
European GameCube version.

Some fixes are not tested on the headset yet: the shield with two-hand weapons,
the gauntlet plates, the bow on Epona, and the hookshot tip.

The wrist HUD needs more work. The wrist panel is small. It also shows when you
raise the shield, because the back of the hand then points at your eyes. We did
not test the minimap on the wrist yet.

## Open decisions

- **Snap turn and Z-targeting** ([#5](https://github.com/gibranfelix/oot-vr/issues/5)). The default is now
  physical turning. The right stick selects items, and the right trigger does
  Z-targeting. Artificial turning is an option. We must decide if a lock-on
  target turns the view by default.

## Where to help

- Any issue in the table above. Each one is small and independent.
- Tests on the Meta Quest 3. We tested only on the Quest 3S.

Before you start, read [`docs/architecture.md`](docs/architecture.md) and
[`CONTRIBUTING.md`](CONTRIBUTING.md).
