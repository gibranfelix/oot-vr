# zelda oot vr

Port de **The Legend of Zelda: Ocarina of Time** al Meta Quest 3S, en primera
persona y standalone — nativo en el visor, sin PC.

Proyecto personal. **Este repo no contiene assets del juego**: hace falta un dump
de tu propio cartucho.

## Créditos

Este port no parte de cero. Se apoya en el trabajo de otra gente:

- **[ShinyWindow](https://github.com/ShinyWindow)** —
  [`Shipwright-VR`](https://github.com/ShinyWindow/Shipwright-VR) y
  [`libultraship-vr`](https://github.com/ShinyWindow/libultraship-vr). La capa de
  VR sale de aquí: la sesión OpenXR, el estéreo por ojo, la cámara en primera
  persona, el combate físico y los cientos de excepciones de control que hicieron
  falta para que OoT funcione en VR. `libultraship-vr` es MIT.
- **[Harbour Masters](https://github.com/HarbourMasters)** —
  [Ship of Harkinian](https://github.com/HarbourMasters/Shipwright), el port de
  OoT sobre el que se construye todo lo demás.
- **[Kenix3](https://github.com/Kenix3)** —
  [`libultraship`](https://github.com/Kenix3/libultraship), MIT.
- **[linkzenic](https://github.com/linkzenic)** —
  [`Shipwright-Android`](https://github.com/linkzenic/Shipwright-Android), la
  referencia de SoH corriendo en Android arm64 con GLES3.
- **[zeldaret](https://github.com/zeldaret/oot)** — la decompilación de OoT.

Zelda y Ocarina of Time son marcas de Nintendo. Este proyecto no está asociado
con Nintendo de ninguna forma.

## El código

Vive en `port/`. Para construir el APK:

```
port/Android/build-apk.sh
```

Luego, con el visor conectado, ver `port/Android/README.md`.

## Por dónde empezar

- `docs/ESTADO.md` — qué está decidido y cuál es el siguiente paso.
- `AGENTS.md` — cómo se trabaja en este repo.
- `CONTEXT.md` — glosario del proyecto.
