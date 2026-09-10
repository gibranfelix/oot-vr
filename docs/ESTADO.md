# Estado del proyecto

Última actualización: 2026-09-10.

## En una frase

Investigación terminada, **destino confirmado** (2026-09-10), cero código
escrito. El siguiente paso son dos experimentos que se pueden hacer en paralelo y
que convierten en observación varias decisiones que hoy son teóricas.

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
- Unirlas son **tres migraciones** antes de tener nada que ejecutar: D3D11→GLES,
  hooks de `gfx_pc.cpp`→`Fast::Interpreter`, y rebasar ~118 call sites de 9.0.0 a
  9.2.3 cruzando ZAPDTR→torch y OTR→O2R.
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
5. **Combate por gesto o por botón — ticket 11.** ShinyWindow trae combate por
   movimiento (`vr-combat/`, ~47 de los ~118 call sites). Decir que no recorta el
   ticket 09 casi a la mitad. Bloqueado por el 03.

Las otras siete decisiones de encuadre se heredan de Mario sin cambio.

## Siguiente paso

**No hay APK del destino que instalar** — el "camino 2" de Mario (probar antes de
construir) no tiene equivalente aquí. Sí existe binario del port *plano*, pero
eso es instrumento de medición, no el destino. Lo más parecido son dos experimentos independientes que
se pueden hacer en paralelo:

- **Ticket 02** — instalar el port Android *flat* de `linkzenic` en el 3S. Corre en
  modo 2D, sin VR. Mide framerate y consumo reales en el visor. **APK ya
  descargado** (`v9.2.3-android.14`, arm64-v8a, en
  `.scratch/oot-quest-3s/artifacts/`): linkzenic sí publica binarios, así que
  este ticket **no depende del 06** (toolchain). Lo que falta es enchufar el
  visor y jugar — checklist exacta en el propio ticket.
- **Ticket 03** — compilar `ShinyWindow/Shipwright-VR` en PC y jugarlo con el
  visor por Link/AirLink. Es PCVR, no es el destino, pero pone a prueba las
  decisiones 2, 4, 5 y 6 con el juego real en primera persona.

Ambos están desbloqueados, junto con el ticket 06 (toolchain). El 03 es el que
más informa: desbloquea el 04, el 05 y el 10.

Los tickets 07, 08 y 09 son las tres migraciones y son semanas de trabajo cada
una. Ahora que el destino está confirmado, lo que los bloquea son el 03 y el 06.

## Cómo trabajar aquí

Lee `AGENTS.md`. El mapa se avanza con `/wayfinder .scratch/oot-quest-3s/map.md`,
que toma el primer ticket de la frontera, lo resuelve y anota la decisión. Nunca
más de un ticket por sesión, salvo los de tipo `research`.
