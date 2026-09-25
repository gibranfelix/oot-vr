# Status

Last update: 2026-09-25.

## What works

- The full main story. We played from Kokiri Forest to Ganon's Tower on a
  Quest 3S.
- Stereo render, first-person camera, and head tracking.
- The physical sword and shield with the Touch controllers.
- Automatic world scale for child Link and adult Link.
- Menus and the pause screen on a floating panel.

## Known problems

| Problem | Issue |
|---|---|
| A gray rectangle shows during scene transitions | [#33](ISSUE:33) |
| A bomb shows as a pink sphere without texture | [#34](ISSUE:34) |
| The shield in your hand blocks the view | [#35](ISSUE:35) |
| A received item stays inside the camera | [#36](ISSUE:36) |
| The cutscene camera goes into the geometry (Rauru) | [#37](ISSUE:37) |
| The game starts two times at each launch | [#38](ISSUE:38) |
| When you put your head into a wall, you can see behind it | [#39](ISSUE:39) |
| Link shouts each time you swing the sword | [#32](ISSUE:32) |
| Houses, shops, and the Market show a gray mesh without texture | [#19](ISSUE:19) |

Some fixes are not tested on the headset yet: the shield with two-hand weapons,
the gauntlet plates, the bow on Epona, and the hookshot tip.

## Open decisions

- **Snap turn and Z-targeting** ([#6](ISSUE:6)). Z-targeting uses the right
  stick for the view direction. Snap turn also needs the right stick. We must
  decide how the two work together.

## Where to help

- Any issue in the table above. Each one is small and independent.
- Tests on the Meta Quest 3. We tested only on the Quest 3S.
- An easier method to make `oot.o2r`. At this time, you must build the host
  tools. This is the most difficult step for players.

Before you start, read [`docs/architecture.md`](docs/architecture.md) and
[`CONTRIBUTING.md`](CONTRIBUTING.md).
