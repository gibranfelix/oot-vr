# Envoltorio Android (Quest 3S)

## Construir el APK

```
port/Android/build-apk.sh
```

Hace, en orden:

1. **Build host** para generar `soh.o2r` (los assets del *port*: shaders, logo,
   texturas del menú). Necesita ZAPD corriendo en el PC, por eso es un build
   aparte. Se salta si `port/soh.o2r` ya existe.
2. **Build arm64** del juego. CMake baja el loader de OpenXR de Maven, fijado por
   hash.
3. Copia `libsoh.so`, `libSDL2.so` (sin símbolos) y `libopenxr_loader.so` a
   `app/libs/arm64-v8a/`, y `soh.o2r` a los assets.
4. **gradle** empaqueta. Nunca ejecuta CMake: fija versiones exactas de NDK y
   CMake (el port de referencia pide 3.31.5) y no hace falta pelearse con eso
   para meter un `.so` que ya compila.

Resultado: `port/Android/app/build/outputs/apk/debug/app-debug.apk`.

## Los assets del juego: `oot.o2r`

Salen de **tu** cartucho y **nunca** entran en el repo ni en el APK. Pon el `.z64`
en `port/OTRExporter/` (el `.gitignore` lo bloquea) y:

```
cmake --build port/build-host --target ExtractAssets
```

Deja `oot.o2r` en `port/`.

## Instalar en el visor

```
adb install -r port/Android/app/build/outputs/apk/debug/app-debug.apk
adb shell mkdir -p /sdcard/Android/data/org.oot.vr/files
adb push port/oot.o2r /sdcard/Android/data/org.oot.vr/files/oot.o2r
```

Y abrir **OoT VR** desde Orígenes desconocidos.

## Si "no arranca"

Casi nunca es el port. Horizon OS se come el lanzamiento sin decir nada cuando:

- **Nadie lleva el visor puesto**: `vrlockscreen/.SensorLockActivity` toma el
  foreground y el lanzamiento queda cacheado. No hay forma de evitarlo por `adb`.
- **Hay un diálogo del sistema abierto** dentro del visor.

Antes de depurar nada:

```
adb shell dumpsys activity activities | grep topResumedActivity
```

Si arriba no está `org.oot.vr`, el problema no es el código.

## Ver qué hace

```
adb logcat -s soh:V              # todo el log del juego; la capa de VR con prefijo [VR]
adb exec-out screencap -p > s.png   # el ojo izquierdo, vía el espejo (sin verificar aún)
```

## Qué se dejó fuera del port de linkzenic, y por qué

Su `MainActivity` son ~1500 líneas: selector de ROM en el dispositivo, diálogo
de extracción y overlay táctil — trece métodos JNI cuya implementación en C vive
en *su* fork. Nada de eso aplica a un build privado en un visor: el ROM se
convierte en el PC y el input entra por OpenXR. Aquí son ~110 líneas.

Lo que **sí** viaja: los 300 KB de configs del extractor. `RunExtract()` exige esa
carpeta en cada arranque aunque ya haya un `oot.o2r`, y sin ella se queda
esperando un clic en un diálogo que en VR no se puede pulsar.
