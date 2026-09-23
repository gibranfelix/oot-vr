# zelda oot vr

Port standalone de **The Legend of Zelda: Ocarina of Time** al Meta Quest 3S, en
primera persona, corriendo nativo en el visor sin PC.

Construido sobre Ship of Harkinian / libultraship. **Este repo no contiene assets
del juego.** Hace falta un dump del cartucho propio; el `.gitignore` bloquea
`*.z64`, `*.otr` y `*.o2r`, y ningún build debe poder producir un artefacto
commiteable con assets dentro.

## Dónde está el código

Todo en **`port/`**: el juego, el motor (`port/libultraship`), las herramientas de
extracción (`port/ZAPDTR`, `port/OTRExporter`) y el envoltorio de Quest
(`port/Android`). No hay submódulos ni forks aparte; es desarrollo nuestro.

- **APK**: `port/Android/build-apk.sh`, una sola orden.
- **Compila desde `/home`, no desde `/mnt/data`**: es NTFS por fuseblk y no
  soporta symlinks ni bit de ejecución.
- **Archivos nuevos dentro de `port/`**: el `.gitignore` de Shipwright es una
  plantilla de Visual Studio que ignora carpetas llamadas `debug/` y `log/`,
  `*.png` y `Makefile`. `libultraship` tiene código real en `src/fast/debug/` y
  `src/libultraship/log/`. Si un archivo nuevo "no aparece" en `git status`, es
  eso: añádelo con `git add -f`.
- **No toques los finales de línea** de `port/libultraship`, `port/ZAPDTR` ni
  `port/OTRExporter`: `port/.gitattributes` los marca `-text` porque algunos son
  parches que se aplican con `git apply` al configurar.

## NO CONFUNDIR con el proyecto de Mario

Existe un proyecto hermano en `../mario-64`: un port
standalone de Super Mario 64 al mismo visor, con el mismo encuadre. **Son dos
esfuerzos separados con mapas separados.** Comparten las restricciones de
plataforma (Horizon OS, OpenXR, GLES, sideloading) y nada más.

Si estás trabajando aquí, no toques el repo de Mario. Si necesitas su
investigación como referencia, hay una copia en
`.scratch/oot-quest-3s/research/reference/sm64-landscape.md`.

## Por dónde empezar

1. `docs/ESTADO.md` — qué está decidido, qué está pendiente, cuál es el siguiente
   paso exacto. **Léelo primero, siempre.**
2. `.scratch/oot-quest-3s/map.md` — el mapa de wayfinder: destino, decisiones de
   encuadre, niebla y fuera de alcance.
3. `.scratch/oot-quest-3s/issues/` — los tickets. La frontera son los que están
   `open`, sin `Blocked by` pendiente y sin asignar.
4. `.scratch/oot-quest-3s/research/` — los hechos verificados. No los
   reinvestigues; si algo cambió, corrige el archivo.

Para avanzar el mapa: `/wayfinder .scratch/oot-quest-3s/map.md`.

## Agent skills

### Issue tracker

Issues live as markdown files under `.scratch/<feature>/`.
See `docs/agents/issue-tracker.md`.

### Triage labels

The five canonical roles, each label equal to its name.
See `docs/agents/triage-labels.md`.

### Domain docs

Single-context: `CONTEXT.md` and `docs/adr/` at the repo root.
See `docs/agents/domain.md`.
