#!/usr/bin/env bash
# Build the Quest APK from a clean checkout: host build for soh.o2r, arm64 build of the game,
# then gradle. Run from anywhere; paths are resolved from this script's location.
#
#   port/Android/build-apk.sh            -> port/Android/app/build/outputs/apk/debug/app-debug.apk
#
# The game's own assets (oot.o2r) are NOT part of this: they come from your cartridge and never
# enter the repository or the APK. See port/Android/README.md for how to push them to the headset.
set -euo pipefail

ANDROID_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PORT="$(cd "$ANDROID_DIR/.." && pwd)"
NDK="${ANDROID_NDK_HOME:-$HOME/Android/Sdk/ndk/26.3.11579264}"
export ANDROID_HOME="${ANDROID_HOME:-$HOME/Android/Sdk}"
JOBS="$(nproc)"

[ -f "$NDK/build/cmake/android.toolchain.cmake" ] || { echo "NDK not found at $NDK (set ANDROID_NDK_HOME)" >&2; exit 1; }

# 1. soh.o2r - the PORT's assets - needs ZAPD running on the host, so it is a separate build.
if [ ! -f "$PORT/soh.o2r" ]; then
    echo "==> host build: soh.o2r"
    cmake -S "$PORT" -B "$PORT/build-host" -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build "$PORT/build-host" --target GenerateSohOtr -j "$JOBS"
fi

# 2. The game for arm64. The OpenXR loader is fetched by CMake (see the root CMakeLists).
echo "==> arm64 build"
cmake -S "$PORT" -B "$PORT/build-quest" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$NDK/build/cmake/android.toolchain.cmake" \
    -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24 -DANDROID_STL=c++_static \
    -DUSE_OPENGLES=ON -DSDL_SHARED=ON -DCMAKE_BUILD_TYPE=Release -DBUILD_REMOTE_CONTROL=0
cmake --build "$PORT/build-quest" -j "$JOBS"

# 3. Stage the three native libraries. Stripped: libsoh.so is ~1.1 GB with debug info, ~42 MB without.
echo "==> staging native libraries"
STRIP="$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip"
LIBS="$ANDROID_DIR/app/libs/arm64-v8a"
mkdir -p "$LIBS"
"$STRIP" --strip-unneeded -o "$LIBS/libsoh.so" "$PORT/build-quest/soh/libsoh.so"
"$STRIP" --strip-unneeded -o "$LIBS/libSDL2.so" "$PORT/build-quest/_deps/sdl2-build/libSDL2.so"
OPENXR_SO="$(sed -n 's/^OPENXR_LOADER_SO:FILEPATH=//p' "$PORT/build-quest/CMakeCache.txt")"
cp "$OPENXR_SO" "$LIBS/libopenxr_loader.so"

mkdir -p "$ANDROID_DIR/app/src/main/assets"
cp "$PORT/soh.o2r" "$ANDROID_DIR/app/src/main/assets/soh.o2r"

# 4. Package. gradle only zips things up here; it never runs CMake (see app/build.gradle).
echo "==> gradle"
(cd "$ANDROID_DIR" && ./gradlew assembleDebug --no-daemon -q)

APK="$ANDROID_DIR/app/build/outputs/apk/debug/app-debug.apk"
echo "==> $APK ($(du -h "$APK" | cut -f1))"
