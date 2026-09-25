# 0003 — Publicar el repo como lo hace la escena de SoH

Fecha: 2026-09-25
Estado: aceptada

## Contexto

El destino (ticket [#3](https://github.com/gibranfelix/zelda-oot-vr/issues/3)) era distribución privada **con la puerta
abierta a publicar si el resultado gusta**. Gustó: el juego corre y se ha jugado
de Kokiri a la torre de Ganon. El usuario quiere abrirlo.

El ADR 0002 dejó escrito que publicar era una decisión aparte, y por qué: el
código de `port/` no tiene una sola licencia.

| Parte | Origen | Licencia |
|---|---|---|
| `port/libultraship` | Kenix3, con la capa VR de ShinyWindow | MIT |
| `port/libultraship/src/fast`, `include/fast` | Emill, MaikelChan | MIT |
| `port/ZAPDTR` | zeldaret | MIT |
| `port/ZAPDTR/lib/libgfxd` | glank | MIT |
| `port/OTRExporter` | Harbour Masters | MIT |
| `port/soh`, y el resto de `port/` | Harbour Masters (Shipwright), con la capa VR de ShinyWindow (`Shipwright-VR`) | **ninguna** |
| `port/Android` | `linkzenic/Shipwright-Android`, adaptado a Quest (ticket [#16](https://github.com/gibranfelix/zelda-oot-vr/issues/16)) | **ninguna** |

Shipwright se apoya a su vez en `zeldaret/oot`, que tampoco tiene licencia.

Software libre en sentido estricto exige que el titular de los derechos dé el
permiso. Sobre `port/soh` nadie lo ha dado, y este proyecto no puede darlo por
ellos.

## Opciones

1. **Publicar como la escena.** Repo público, lo nuestro bajo MIT y un aviso
   honesto de qué partes no tienen licencia.
2. **Pedir permiso upstream.** Viable con ShinyWindow, que es una persona. No con
   Shipwright, que tiene cientos de contribuidores.
3. **Publicar solo lo nuestro**, como parche o módulo MIT que cada quien aplica
   sobre SoH. Lo más limpio legalmente, pero rompe lo que compró el ADR 0002: un
   clon que compila con una sola orden.

## Decisión

**La 1.** Es lo que hacen ShinyWindow, linkzenic, roborich y skijer: forks
públicos de un repo sin licencia, con releases, desde hace años. La norma de
facto de la escena no coincide con la lectura estricta, y este proyecto se pone
del lado de la norma sabiendo que lo hace.

Lo que eso significa en concreto:

- `LICENSE` en la raíz: MIT, **acotada a lo que escribió este proyecto**. El
  propio archivo dice su alcance, así que GitHub no lo detectará como "MIT" a
  secas — y está bien, porque el repo entero no lo es.
- `NOTICE.md`: la tabla de arriba, para quien vaya a reutilizar algo.
- Los `LICENSE` MIT que ya existen dentro de `port/` no se tocan.
- El proyecto **no se llama software libre**. Se dice "código abierto" con el
  aviso delante, o simplemente "público".

## Lo que queda antes de cambiar la visibilidad

- **El nombre.** El repo se llama `zelda-oot-vr`. El riesgo real de un aviso
  DMCA de Nintendo está en el nombre y en los binarios, no en el código. Se
  renombra a algo sin la marca antes de hacerlo público.
- **Los assets**: revisados el 2026-09-25. El historial no tiene `.z64`, `.o2r`,
  `.apk` ni keystores. Las texturas de `port/OTRExporter/assets/textures`
  (`nintendo_rogo_static`, `title_static`) son de SoH — el logo del barco y los
  menús de Boss Rush, MQ y RAND —, no de Nintendo.
- **Qué documentación interna se publica**: `.scratch/`, `AGENTS.md` y los
  tickets. Nada de eso es secreto, pero está escrito para trabajar, no para
  leerse desde fuera.
- **Un clon limpio que compile en otra máquina.**
- **Releases**: si se publican APKs, solo con `soh.o2r`. Nunca con nada que salga
  del ROM; el diseño actual ya lo impide.

## Lo que se pierde

- La posibilidad de llamarlo software libre sin mentir.
- Si Harbour Masters o Nintendo piden retirarlo, el repo se retira. Con la
  opción 3 solo se retiraría el parche si lo pidiera ShinyWindow o Harbour
  Masters.
