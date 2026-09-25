# Licenses by directory

This repository contains code from many sources. Not all of this code has a
license. Before you use a part of it, read this file.

| Directory | Source | License |
|---|---|---|
| All files outside `port/` | This project | MIT (`LICENSE`) |
| `port/libultraship` | [Kenix3](https://github.com/Kenix3/libultraship), VR layer from [ShinyWindow](https://github.com/ShinyWindow/libultraship-vr) | MIT (`port/libultraship/LICENSE`) |
| `port/libultraship/src/fast`, `port/libultraship/include/fast` | Emill, MaikelChan (Fast3D) | MIT (its own `LICENSE.txt`) |
| `port/ZAPDTR` | [zeldaret](https://github.com/zeldaret) | MIT (`port/ZAPDTR/LICENSE`) |
| `port/ZAPDTR/lib/libgfxd` | glank | MIT (its own `LICENSE`) |
| `port/OTRExporter` | [Harbour Masters](https://github.com/HarbourMasters) | MIT (`port/OTRExporter/LICENSE`) |
| `port/soh` and all other files in `port/` | [Ship of Harkinian](https://github.com/HarbourMasters/Shipwright), VR layer from [`Shipwright-VR`](https://github.com/ShinyWindow/Shipwright-VR) | **No license** |
| `port/Android` | [`linkzenic/Shipwright-Android`](https://github.com/linkzenic/Shipwright-Android), changed for the Quest | **No license** |

Ship of Harkinian uses the decompilation
[`zeldaret/oot`](https://github.com/zeldaret/oot). The decompilation also has no
license.

**No license** means that the authors did not give permission to copy or
distribute the code. This repository publishes that code in the same way as the
other public forks of Ship of Harkinian. This does not give you permission to
use that code.

This project also changed files in those directories. Our changes are MIT. The
original code in the same files keeps the rights of its authors.

## Game assets

This repository does not contain assets of Ocarina of Time. The build makes
`soh.o2r` from assets that Ship of Harkinian owns. You make `oot.o2r` from a
dump of your own cartridge. Do not put `oot.o2r` in the repository or in a
release.

Zelda and Ocarina of Time are trademarks of Nintendo. This project has no
connection with Nintendo.
