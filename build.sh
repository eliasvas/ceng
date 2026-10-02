#!/usr/bin/env bash
set -e

CC="clang"

CFLAGS="-std=gnu23 -Wall -Wextra \
-Wno-unused-function -Wno-unused-parameter \
-Wswitch-enum -fno-exceptions -fstack-protector \
-g -fPIC -O0"
INC="-Iext -Isrc/frz -Isrc -Idemo -Isrc/demo -Isrc/demo/entity"
CLIBS="-lGL -lm"

rm -rf build
mkdir -p build/obj

# -------------------------
# gamelib
# -------------------------

for src in src/demo/*.c src/demo/entity/*.c src/asset/*.c; do
  "$CC" $CFLAGS $INC -c "$src" -o "build/obj/$(basename "${src%.c}.o")"
done
"$CC" -shared build/obj/*.o -o build/libgame.so
rm -rf build/obj/*.o

# -------------------------
# engine
# -------------------------
for src in src/core/*.c src/rend/*.c src/particle/*.c src/gui/*.c src/platform/platform_sdl3.c; do
  "$CC" $CFLAGS $INC -c "$src" -o "build/obj/$(basename "${src%.c}.o")"
done

"$CC" \
build/obj/*.o \
$(pkg-config --cflags --libs sdl3 sdl3-image sdl3-sound sdl3-ttf) \
$CLIBS \
-Lbuild \
-lgame \
-Wl,-rpath,'$ORIGIN' \
-o build/ceng
