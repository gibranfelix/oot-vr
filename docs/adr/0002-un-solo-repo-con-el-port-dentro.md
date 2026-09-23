# 0002 — Un solo repositorio, con el port dentro

Fecha: 2026-09-23
Estado: aceptada

## Contexto

El port se construyó forkeando el trabajo de ShinyWindow (ticket 12): el juego
en `Shipwright-VR@MotionControls-2`, el motor en `libultraship-vr@vr-port`, y
`ZAPDTR` / `OTRExporter` como submódulos de HarbourMasters. Cuatro repositorios
con commits nuestros encima, más este, que solo tenía el mapa y las decisiones.

Vivían en `~/src/oot-vr/`, sin respaldo, con los submódulos apuntando a rutas del
disco. Un clon limpio no compilaba en ninguna otra máquina.

Esa forma solo tiene sentido si se va a **seguir sincronizado con upstream** —
es lo que compran los forks y los submódulos: poder hacer `git merge` del
original. Y eso ya estaba descartado: el ticket 08 midió que upstream migró a
SDL3 (#1191) y metió un sistema de componentes de 232 archivos (#1174), mientras
que el juego y el envoltorio Android son SDL2. Subir a upstream es una migración,
no un merge.

## Decisión

**El código del juego vive en este repositorio, en `port/`**, como código
propio. Sin forks aparte y sin submódulos.

Se importó con `git archive` — solo lo versionado — así que el ROM, los `.o2r` y
los builds quedaron fuera por construcción. Los tres subárboles que eran
submódulos son idénticos byte a byte a su origen; `port/.gitattributes` los marca
`-text` para que git no les toque los finales de línea.

## Lo que se pierde

- **El historial de commits de los forks.** El código entró como una
  instantánea. Los *porqués* de cada cambio siguen en los tickets 07, 08, 13 y
  14; los mensajes de commit individuales solo existen en `~/src/oot-vr/`.
- **Traer un arreglo de upstream deja de ser un `git merge`.** Pasa a ser un
  cherry-pick a mano o un parche. El ticket 08 midió que la superficie de VR en
  `interpreter.cpp` son 8 funciones localizadas, así que un arreglo puntual sigue
  siendo barato mientras esas 8 no se muevan.
- **La atribución deja de ser estructural.** En un fork, GitHub dice de dónde
  viene. Aquí lo dice el `README`, que acredita a ShinyWindow, Harbour Masters,
  Kenix3, linkzenic y zeldaret. Mantenerlo es obligación, no cortesía.

## Consecuencias

- Un clon compila con `port/Android/build-apk.sh`, sin nada de fuera salvo el
  SDK/NDK de Android. El loader de OpenXR lo baja CMake de Maven, fijado por
  hash.
- El `.gitignore` de Shipwright es una plantilla de Visual Studio que ignora
  carpetas `debug/` y `log/`. Al aplanar los submódulos habría descartado en
  silencio 23 archivos de código real de `libultraship`. Están forzados; los
  archivos nuevos en esas carpetas hay que añadirlos con `git add -f`
  (`AGENTS.md`).
- **La licencia no cambia.** Shipwright y zeldaret/oot no tienen licencia; la
  exposición es la misma que con el fork (ticket 12). El repositorio es
  **privado**, como el destino. Publicar sigue siendo una decisión aparte.
