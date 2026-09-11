# Estado del proyecto

Última actualización: 2026-09-10.

## En una frase

**Hay APK.** El juego entero compila para arm64 con la capa de VR sobre GLES y
OpenXR, y sale empaquetado como app inmersiva de Quest. Lo único que separa de
saber si funciona es **enchufar el 3S**.

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
  **Hecha el 2026-09-10** (ticket 07): ~350 líneas de 2822, detrás de una costura
  de diez funciones, compilando para arm64. Antes de eso se había corregido de
  ~~tres~~ a una, al descubrir que el juego usa la rama **`vr-port`** y no
  `vr-integration`: los hooks ya están en `Fast::Interpreter`, el juego ya está
  en SoH 9.2.3 con `.o2r`, y el cruce ZAPDTR→torch ni siquiera existe.
  Lo que de verdad falta ahora es el **envoltorio Android** (ticket 14).
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

1. ~~**Confirmar el destino — ticket 01.**~~ **Resuelto 2026-09-10.** Standalone
   confirmado, PCVR descartado como destino (`docs/adr/0001`). El destino se
   reescribió: suelo práctico en vez de "sin crashes", línea principal hasta
   créditos, y la comodidad **dentro** del criterio de éxito. Ver `map.md` y
   `CONTEXT.md`.
2. **Decisión 5 — snap turn contra Z-targeting.** En SM64 el stick derecho estaba
   libre. En OoT el Z-targeting ya ocupa el rol de "hacia dónde miro", con siete
   modos de cámara, y con lock-on el marco de referencia del stick tiene que
   cambiar de cabeza a cuerpo. Hay que re-decidir. Ticket 04.
3. **Decisión 6 — escala adulto/niño.** Dos alturas y una transición a mitad de
   partida. Hay que re-decidir. Ticket 05.

4. **Alcance mínimo de comodidad — ticket 10.** Graduó de la niebla al confirmar
   el destino. Bloqueado por el 03.
5. **Combate por gesto o por botón — ticket 11.** No es solo gesto: `vr-port`
   trae `vr_physics.cpp` (77 KB), una espada con física de verdad. Bloqueado por
   el 03.
6. ~~**Forkear `MotionControls-2` o reimplantar — ticket 12.**~~ **Resuelto
   2026-09-10**: forkear, citando a ShinyWindow. El argumento de licencia para
   reimplantar no se sostenía.

Las otras siete decisiones de encuadre se heredan de Mario sin cambio.

## Dónde está el código

Dos forks, en `~/src/oot-vr/`:

- **`soh-vr`**, rama `quest` (`1893b98e`) — el juego. Su submódulo `libultraship`
  apunta al de al lado.
- **`lus-vr`**, rama `quest-gles` (`05b0381`) — la capa de VR portada a GLES.

Al lado, solo lectura: `soh-android` y `lus-android` (el port plano de linkzenic),
`openxr-android` (el AAR del loader de Khronos).

**Ese árbol no está respaldado.** Las copias durables viven en este repo, en
`.scratch/oot-quest-3s/artifacts/`: los parches de los tickets 07 y 14.

**El APK**: `~/src/oot-vr/soh-vr/Android/app/build/outputs/apk/debug/app-debug.apk`,
con copia en `/tmp/ootvr-quest-debug.apk`. 32 MB, arm64-v8a, firmado de debug.

Cómo reconstruir: `soh-vr/Android/README.md`. Son tres pasos — build host (genera
`soh.o2r`), build nativo arm64, y gradle. El nativo se hace **fuera de gradle** a
propósito: gradle fija versiones exactas de NDK y CMake que esta máquina no tiene.

**Riesgo abierto**: el path D3D11 se refactorizó junto con el port del ticket 07 y
no se ha recompilado — aquí no hay MSVC. No afecta al destino (Windows no es
objetivo), pero está roto hasta que se demuestre lo contrario.

## Siguiente paso

**El ticket 03 (PCVR por Link) quedó fuera de alcance** el 2026-09-10: exigía
reiniciar a Windows y el usuario no quiere. Con él se fue el banco de pruebas, y
las cuatro decisiones que informaba pasan al ticket 13 — se deciden sobre el
build propio, después de la migración.

El **ticket 08** se cerró el 2026-09-10 sin trabajo de código: **no se rebasa**.
`vr-port` está 53 commits *por delante* del LUS que trae SoH 9.2.3, los 21 hooks
no colisionan con nada de lo que upstream ha movido, y upstream ya migró a SDL3
mientras el juego y el envoltorio Android siguen en SDL2.

El **ticket 14** se cerró el 2026-09-10: hay APK de Quest, y con él cayó el
último bloqueante del 13.

Queda un solo camino, y **es físico**:

- **Ticket 13** — primer arranque en el 3S. Instalar el APK, empujar tu `oot.o2r`,
  abrirlo. Desbloquea las cuatro decisiones pendientes: snap turn, escala,
  comodidad y combate.
- **Ticket 02** — en paralelo, medir framerate con el port plano de linkzenic.
  APK ya descargado.

Los dos necesitan exactamente lo mismo y nada más.

**Restricción de encuadre nueva: este proyecto es SDL2.** Subir LUS a
`upstream/main` significa migrar el juego y el envoltorio a SDL3, que es un
esfuerzo aparte y no un `git merge`. Ver el ticket 08.

**Todo lo que se puede preparar sin visor, está preparado**: el APK en
`/tmp/ootvr-quest-debug.apk` y el `oot.o2r` de tu cartucho en `/tmp/oot.o2r`
(PAL GC, versión soportada). Tres comandos de `adb` y a jugar — están en el
ticket 13.

**El cuello de botella es el visor.** `adb devices` sigue sin verlo — ni siquiera
aparece en el bus USB. Ya no queda trabajo de código por delante: modo
desarrollador en el 3S, USB-C, aceptar el diálogo dentro del visor, y
`adb install -r /tmp/ootvr-quest-debug.apk`.

Antes de eso se barrieron los fallos de arranque que **sí** se podían ver sin
hardware, mirando la tabla de símbolos del `.so`. Apareció uno de verdad:
`libsoh.so` exportaba `main` donde SDL iba a pedir `SDL_main`, lo que habría
cerrado la app al instante sin dejar rastro en un build correcto. Corregido. El
resto salió limpio: el loader de OpenXR está en `DT_NEEDED`, los 558 símbolos
indefinidos resuelven todos, VR arranca sola, y si OpenXR falla el juego cae a
plano en vez de crashear.

Si el primer arranque falla, **no será por el empaquetado**. Los sospechosos que
quedan son el estéreo, el contexto EGL y la sesión OpenXR sobre GLES — lo que el
ticket 07 escribió y nadie ha visto correr.

## Cómo trabajar aquí

Lee `AGENTS.md`. El mapa se avanza con `/wayfinder .scratch/oot-quest-3s/map.md`,
que toma el primer ticket de la frontera, lo resuelve y anota la decisión. Nunca
más de un ticket por sesión, salvo los de tipo `research`.
