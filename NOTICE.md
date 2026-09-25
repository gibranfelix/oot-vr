# Licencias por directorio

Este repo junta código de varias fuentes, y no todo tiene licencia. Si vas a
reutilizar algo, mira aquí primero. El porqué de publicarlo así está en
`docs/adr/0003`.

| Directorio | Origen | Licencia |
|---|---|---|
| todo lo que está fuera de `port/` | este proyecto | MIT (`LICENSE`) |
| `port/libultraship` | [Kenix3](https://github.com/Kenix3/libultraship), capa VR de [ShinyWindow](https://github.com/ShinyWindow/libultraship-vr) | MIT (`port/libultraship/LICENSE`) |
| `port/libultraship/src/fast`, `port/libultraship/include/fast` | Emill, MaikelChan (Fast3D) | MIT (`LICENSE.txt` propio) |
| `port/ZAPDTR` | [zeldaret](https://github.com/zeldaret) | MIT (`port/ZAPDTR/LICENSE`) |
| `port/ZAPDTR/lib/libgfxd` | glank | MIT (`LICENSE` propio) |
| `port/OTRExporter` | [Harbour Masters](https://github.com/HarbourMasters) | MIT (`port/OTRExporter/LICENSE`) |
| `port/soh` y el resto de `port/` | [Ship of Harkinian](https://github.com/HarbourMasters/Shipwright), capa VR de [`Shipwright-VR`](https://github.com/ShinyWindow/Shipwright-VR) | **sin licencia** |
| `port/Android` | [`linkzenic/Shipwright-Android`](https://github.com/linkzenic/Shipwright-Android), adaptado a Quest | **sin licencia** |

Ship of Harkinian se apoya en la decompilación
[`zeldaret/oot`](https://github.com/zeldaret/oot), que tampoco tiene licencia.

**Sin licencia** quiere decir que sus autores no han dado permiso para copiarlo
ni redistribuirlo. Este repo lo publica igual que el resto de la escena de SoH,
pero eso no te da a ti ningún permiso sobre ese código. Los cambios que este
proyecto hizo dentro de esos directorios, a partir del commit `a9b68e4`, sí son
MIT por nuestra parte, aunque no por la de los autores originales.

## Assets del juego

Este repo **no contiene assets de Ocarina of Time**. `soh.o2r` se genera con
recursos propios de Ship of Harkinian; `oot.o2r` sale del dump de tu propio
cartucho y nunca debe acabar en el repo ni en una release.

Zelda y Ocarina of Time son marcas de Nintendo. Este proyecto no está asociado
con Nintendo de ninguna forma.
