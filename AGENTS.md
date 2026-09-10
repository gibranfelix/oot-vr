# zelda oot vr

Port standalone de **The Legend of Zelda: Ocarina of Time** al Meta Quest 3S, en
primera persona, corriendo nativo en el visor sin PC.

Construido sobre Ship of Harkinian / libultraship. **Este repo no contiene assets
del juego.** Hace falta un dump del cartucho propio; el `.gitignore` bloquea
`*.z64`, `*.otr` y `*.o2r`, y ningún build debe poder producir un artefacto
commiteable con assets dentro.

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
