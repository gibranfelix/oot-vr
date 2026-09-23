# Estado del proyecto

Última actualización: 2026-09-10.

## En una frase

**Funciona.** Ocarina of Time corre en el Quest 3S, en estéreo, en primera
persona, nativo. Sesión OpenXR en `FOCUSED` a ~82 fps, con glitches gráficos por
clasificar (ticket [#17](https://github.com/gibranfelix/zelda-oot-vr/issues/17)) y las cuatro decisiones de diseño ya desbloqueadas.

## Qué se sabe (verificado, con fuentes en `research/landscape.md`)

**El veredicto**: OoT es **mejor candidato que SM64** para un port VR — 4 de las 9
decisiones de encuadre son más fáciles, 3 iguales, 3 más difíciles — **pero el
primer hito es más largo**, porque nadie ha hecho standalone todavía.

**Lo que hace a OoT mejor:**

- `Camera_Subj3` (`z_camera.c:4368`) es una cámara en primera persona **en
  vanilla**, anclada al hueso de la cabeza vía `Actor_GetFocus`, con
  `Camera_BGCheck` y escalada por `Player_GetHeight()`. En SM64 esto solo existe
  en un fork.
- El acoplamiento cámara→control es **un getter**, no un acceso a campo:
  `Player_GetMovementSpeedAndYaw` (`z_player.c:4145`) es el embudo único de las
  26 acciones de locomoción, y Player nunca lee campos de `struct Camera`. El mod
  VR existente lo resolvió en **~25 líneas**; al fork de SM64 le costó ~120.
- Las cutscenes ya usan **sub-cámara distinta** (`z_demo.c`), así que "caer a
  tercera persona en cutscene" se vuelve un predicado, no una enumeración.
- **Licencia**: `libultraship` y sus forks VR son **MIT**. Se puede *copiar
  código*, no solo la forma. El fork VR de SM64 no tenía licencia.

**Lo que hace a OoT más caro:**

- **No existe port standalone.** Existen las dos mitades y no se hablan:
  - `linkzenic/Shipwright-Android` — arm64-v8a + GLES3 + NDK 26 + targetSdk 33
    sobre SoH 9.2.3, pero con `SDLActivity` dueña de la ventana.
  - `ShinyWindow/libultraship-vr@vr-integration` — OpenXR real (la descripción
    del repo dice OpenVR y **miente**), dos pasadas por ojo, primera persona,
    roomscale, manos trackeadas… pero `XR_USE_GRAPHICS_API_D3D11`, sobre LUS
    pre-refactor y SoH 9.0.0.
  - `HarbourMasters/Shipwright` **no tiene Android en absoluto**. Las releases
    9.2.1 y 9.2.3 publican solo Linux/Mac/Win64.
- ~~Unirlas es **una migración** antes de tener nada que ejecutar: D3D11→GLES.~~
  **Hecha el 2026-09-10** (ticket [#9](https://github.com/gibranfelix/zelda-oot-vr/issues/9)): ~350 líneas de 2822, detrás de una costura
  de diez funciones, compilando para arm64. Antes de eso se había corregido de
  ~~tres~~ a una, al descubrir que el juego usa la rama **`vr-port`** y no
  `vr-integration`: los hooks ya están en `Fast::Interpreter`, el juego ya está
  en SoH 9.2.3 con `.o2r`, y el cruce ZAPDTR→torch ni siquiera existe.
  Lo que de verdad falta ahora es el **envoltorio Android** (ticket [#16](https://github.com/gibranfelix/zelda-oot-vr/issues/16)).
- **La escala no es constante**: `Player_GetHeight()` = 68.0f adulto / 44.0f niño,
  y el juego cambia entre las dos a mitad de partida.

**Hipótesis falsada — importante:** se asumió que `libultraship` ya abstraía la
proyección y que eso haría el port radicalmente más barato. **Es falso.**
`Fast::GfxRenderingAPI::DrawTriangles()` recibe triángulos ya en clip space, y un
grep de `stereo|openxr|multiview|eye` sobre LUS entero da cero. Es el mismo
linaje que `gfx_pc.c`. El punto de intercepción es `Interpreter::GfxSpMatrix`
(`src/fast/interpreter.cpp:1541`), mismo coste que en SM64. Lo que LUS **sí**
aporta es framebuffers de primera clase en el backend, que hacen el render por
ojo a FBO trivial.

**Corolario que solo apareció al portar** (2026-09-10): `vr-port` **ya traía**
`option(USE_OPENGLES "Enable GLES3")` y `cmake/dependencies/android.cmake`, y su
matriz de proyección ya estaba en convención GL. La capa de VR entera estaba tras
`#ifdef ENABLE_DX11`, que solo se define en Windows — no había que cambiar de
backend, había que dejar de compilarla a stubs.

## Qué falta decidir (bloquea el trabajo real)

1. ~~**Confirmar el destino — ticket [#3](https://github.com/gibranfelix/zelda-oot-vr/issues/3).**~~ **Resuelto 2026-09-10.** Standalone
   confirmado, PCVR descartado como destino (`docs/adr/0001`). El destino se
   reescribió: suelo práctico en vez de "sin crashes", línea principal hasta
   créditos, y la comodidad **dentro** del criterio de éxito. Ver [el mapa](https://github.com/gibranfelix/zelda-oot-vr/issues/2) y
   `CONTEXT.md`.
2. **Decisión 5 — snap turn contra Z-targeting.** En SM64 el stick derecho estaba
   libre. En OoT el Z-targeting ya ocupa el rol de "hacia dónde miro", con siete
   modos de cámara, y con lock-on el marco de referencia del stick tiene que
   cambiar de cabeza a cuerpo. Hay que re-decidir. Ticket [#6](https://github.com/gibranfelix/zelda-oot-vr/issues/6).
3. **Decisión 6 — escala adulto/niño.** Dos alturas y una transición a mitad de
   partida. Hay que re-decidir. Ticket [#7](https://github.com/gibranfelix/zelda-oot-vr/issues/7).

4. **Alcance mínimo de comodidad — ticket [#12](https://github.com/gibranfelix/zelda-oot-vr/issues/12).** Graduó de la niebla al confirmar
   el destino. Bloqueado por el [#5](https://github.com/gibranfelix/zelda-oot-vr/issues/5).
5. **Combate por gesto o por botón — ticket [#13](https://github.com/gibranfelix/zelda-oot-vr/issues/13).** No es solo gesto: `vr-port`
   trae `vr_physics.cpp` (77 KB), una espada con física de verdad. Bloqueado por
   el [#5](https://github.com/gibranfelix/zelda-oot-vr/issues/5).
6. ~~**Forkear `MotionControls-2` o reimplantar — ticket [#14](https://github.com/gibranfelix/zelda-oot-vr/issues/14).**~~ **Resuelto
   2026-09-10**: forkear, citando a ShinyWindow. El argumento de licencia para
   reimplantar no se sostenía.

Las otras siete decisiones de encuadre se heredan de Mario sin cambio.

## Dónde está el código

**Aquí, en `port/`.** Es el juego entero: Ship of Harkinian con la capa de VR, el
motor (`port/libultraship`), las herramientas de extracción (`port/ZAPDTR`,
`port/OTRExporter`) y el envoltorio de Quest (`port/Android`).

Hasta el 2026-09-23 vivía repartido en forks locales bajo `~/src/oot-vr/`, con
submódulos que apuntaban a rutas del disco. Se consolidó porque esos repos solo
servían para seguir sincronizados con upstream, y el ticket [#10](https://github.com/gibranfelix/zelda-oot-vr/issues/10) ya había decidido
no hacerlo (upstream migró a SDL3; este proyecto es SDL2). Los tres subárboles
que eran submódulos se importaron **byte a byte** — verificado comparando el árbol
completo — y un `.gitattributes` en `port/` impide que git les normalice los
finales de línea.

`~/src/oot-vr/` ya no hace falta. El historial de cómo se llegó aquí queda en los
tickets [#9](https://github.com/gibranfelix/zelda-oot-vr/issues/9), [#10](https://github.com/gibranfelix/zelda-oot-vr/issues/10), [#15](https://github.com/gibranfelix/zelda-oot-vr/issues/15) y [#16](https://github.com/gibranfelix/zelda-oot-vr/issues/16). Los parches de `.scratch/oot-quest-3s/artifacts/` que
citan esos tickets se retiraron al consolidar — eran el respaldo de los forks, y
`port/` los contiene enteros. Siguen en el historial de git si hacen falta.

**Construir el APK**, de una sola orden:

```
port/Android/build-apk.sh
```

Hace el build host (genera `soh.o2r`), el build arm64, empaqueta con gradle y deja
el APK en `port/Android/app/build/outputs/apk/debug/app-debug.apk`. El loader de
OpenXR lo baja CMake de Maven, fijado por hash: el build no depende de nada fuera
del repo salvo el SDK/NDK de Android.

**Ojo**: el checkout principal está en `/mnt/data`, que es NTFS por fuseblk, y
ahí no se puede compilar (sin symlinks ni bit de ejecución — ver ticket [#8](https://github.com/gibranfelix/zelda-oot-vr/issues/8)).
Compilar desde un clon o worktree en `/home`.

**Riesgo abierto**: el path D3D11 se refactorizó junto con el port del ticket [#9](https://github.com/gibranfelix/zelda-oot-vr/issues/9) y
no se ha recompilado — aquí no hay MSVC. No afecta al destino (Windows no es
objetivo), pero está roto hasta que se demuestre lo contrario.

## Siguiente paso

El **ticket [#15](https://github.com/gibranfelix/zelda-oot-vr/issues/15) se cerró el 2026-09-23**: el juego arranca y se juega. Con él caen
los bloqueantes de las cuatro decisiones pendientes.

Frontera actual, toda takeable:

- **Ticket [#17](https://github.com/gibranfelix/zelda-oot-vr/issues/17)** — clasificar los glitches gráficos. Es el primero que se resuelve
  mirando, no razonando. Cada síntoma tiene su sospechoso identificado (sesgo de
  profundidad, sRGB, HUD, proyección).
- **Tickets [#6](https://github.com/gibranfelix/zelda-oot-vr/issues/6), [#7](https://github.com/gibranfelix/zelda-oot-vr/issues/7), [#12](https://github.com/gibranfelix/zelda-oot-vr/issues/12), [#13](https://github.com/gibranfelix/zelda-oot-vr/issues/13)** — snap turn, escala niño/adulto, comodidad y combate.
  Son HITL: se deciden jugando, no discutiendo.
- **Ticket [#4](https://github.com/gibranfelix/zelda-oot-vr/issues/4)** — medir framerate con el port plano de referencia.

**Y todos necesitan lo mismo: alguien con el visor puesto.** Horizon OS bloquea
el lanzamiento en cuanto nadie lo lleva (`vrlockscreen/.SensorLockActivity`), y
`oculus_proximity_sensor_enabled=0` se ignora incluso tras reiniciar. No hay
camino por `adb`.

## Herramientas que ahora existen

Un visor no tiene consola ni depurador. Durante el arranque se añadieron dos
cosas sin las cuales depurar ahí es a ciegas:

- **`android_sink` de spdlog + nivel `info` en Android**: todo `[VR]` sale por
  `adb logcat -s soh:V`.
- **Espejo del ojo izquierdo en GLES**: `adb exec-out screencap -p` debería
  devolver lo que el visor pinta. **Sin verificar en dispositivo todavía.**

## Cómo trabajar aquí

Lee `AGENTS.md`. El mapa se avanza con `/wayfinder https://github.com/gibranfelix/zelda-oot-vr/issues/2`,
que toma el primer ticket de la frontera, lo resuelve y anota la decisión. Nunca
más de un ticket por sesión, salvo los de tipo `research`.
