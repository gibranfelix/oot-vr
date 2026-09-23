# 0001 — Standalone en el visor, no PCVR

Fecha: 2026-09-10
Estado: aceptada

## Contexto

Existe `ShinyWindow/Shipwright-VR@MotionControls-2`: OoT en VR, en primera
persona, **funcionando hoy**, en PCVR sobre D3D11. Se compila en el PC y se juega
con el Quest 3S por Link o AirLink. Es código de otro, razonado y comentado, y
está a un fin de semana de distancia.

El destino de este proyecto —OoT standalone, nativo en el visor— cuesta **tres
migraciones** por encima de eso: `vr_openxr` de D3D11 a GLES, los hooks de
`gfx_pc.cpp` a `Fast::Interpreter`, y rebasar ~118 call sites de SoH 9.0.0 a
9.2.3. Semanas cada una, y no hay nada ejecutable hasta que las tres estén.

Lo único que compran esas tres migraciones es **quitar el PC de en medio**. El
juego que sale al final es el mismo juego.

La victoria declarada por el usuario es jugar OoT él mismo en primera persona en
su visor. PCVR ya la satisface. La pregunta era honesta: ¿por qué no parar ahí?

## Decisión

Standalone. El PCVR queda como **banco de pruebas**, no como destino.

## Razón

OoT son 25-30 horas. Un juego largo no se juega en dos sesiones épicas: se juega
en cuarenta ratos. PCVR mete un peaje de arranque en cada uno de esos ratos —
encender el PC, lanzar Link, el cable— y ese peaje se paga cuarenta veces.

Es exactamente la fricción que mata las partidas largas: no impide jugar, impide
volver. Sobre un juego de dos horas sería irrelevante; sobre uno de treinta es la
diferencia entre un juego que juegas y uno que arrancas dos veces y abandonas.

El usuario lo dijo sin rodeos: *"comodidad, quiero jugarlo sin depender de la
PC"*.

## Consecuencias

- Las tres migraciones (tickets [#9](https://github.com/gibranfelix/zelda-oot-vr/issues/9), [#10](https://github.com/gibranfelix/zelda-oot-vr/issues/10), [#11](https://github.com/gibranfelix/zelda-oot-vr/issues/11)) están dentro del alcance y son el
  grueso del trabajo.
- `ShinyWindow/Shipwright-VR` se usa igual, pero como **referencia y banco de
  pruebas** (ticket [#5](https://github.com/gibranfelix/zelda-oot-vr/issues/5)), no como base a mejorar.
- El presupuesto de rendimiento pasa a ser crítico: standalone significa que el
  3S renderiza dos pasadas por su cuenta, sin un PC detrás. Lo mide el ticket [#4](https://github.com/gibranfelix/zelda-oot-vr/issues/4).
- Si el 3S no diera para dos pasadas, eso **redibuja la ruta** (resolución,
  foveación, recortes), no el destino. Volver a PCVR sería redibujar el destino y
  sería un esfuerzo nuevo, no una continuación.
