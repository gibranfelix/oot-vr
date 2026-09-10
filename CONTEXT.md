# Contexto del dominio

Glosario del proyecto. Sin detalles de implementación: los términos, no cómo se
construyen. Las decisiones difíciles de revertir viven en `docs/adr/`.

## Destino

El estado en el que este esfuerzo se declara terminado. Está escrito en
`.scratch/oot-quest-3s/map.md` y se confirmó el 2026-09-10. No es una lista de
tareas ni una fecha: es una descripción de cómo se ve la victoria.

## Standalone

El juego corre **nativo en el visor**, sin PC. Es el eje del destino y lo que lo
distingue de las alternativas que ya existen. Ver `docs/adr/0001`.

## PCVR

El juego corre en un PC y el visor es una pantalla conectada por cable o Link.
En este proyecto **no es un destino, es un banco de pruebas**: sirve para poner
decisiones de diseño delante del usuario antes de pagar el coste de construir.

## Suelo práctico

El criterio de calidad del destino. Lo que importa es **llegar al final**, no la
ausencia de fallos: guardar seguido, recargar tras un cuelgue y esquivar algún
punto roto son aceptables. Se eligió frente a "sin crashes" porque sobre un juego
de 25-30 horas encadenadas ese listón no es evaluable hasta el final.

## Línea principal

El alcance del destino: Deku, Dodongo, Jabu, los cinco templos de adulto, Ganon.
**El contenido opcional no es criterio de éxito** — ni las Gold Skulltulas, ni
las máscaras, ni los minijuegos de precisión. Que un minijuego resulte inviable
en VR no invalida el proyecto.

## Comodidad

Ambiguo en este proyecto y conviene no mezclarlos:

- **Comodidad de uso**: no depender de un PC para jugar. Es la razón del
  standalone.
- **Comodidad en VR**: no marearse. Es **criterio de éxito** del destino, medido
  contra el usuario y no contra un estándar externo: sesiones de una hora sin
  malestar.

Cuando el mapa o un ticket dice "comodidad" a secas, se refiere al segundo.

## Distribución privada

El artefacto es para el usuario, no para publicar. **Publicar sigue siendo una
opción abierta** si el resultado gusta, y es esa puerta la que hace obligatoria
la disciplina de licencias: se copia código solo de lo MIT (`libultraship` y sus
forks); de lo que no tiene licencia (`Shipwright`, `zeldaret/oot`) se copia la
forma y lo que explican los comentarios, nunca el texto.

## Assets del port / assets del juego

Dos cosas distintas que viajan por caminos distintos. `soh.o2r` son los assets
del **port** y van dentro del APK. `oot.o2r` son los del **juego** y los genera
el usuario desde su propio cartucho. Este repo nunca contiene los segundos.
