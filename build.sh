#!/usr/bin/env bash
set -e

# TODO: Maybe we could add the asset handling here

# -----------------------------
# Parse arguments
# Usage: ./build.sh gd=../my_game od=out clean=0
# -----------------------------
for arg in "$@"; do
  case $arg in
  gd=*)
    GAME_DIR="${arg#*=}"
    ;;
  od=*)
    OUTPUT_DIR="${arg#*=}"
    ;;
  clean=*)
    CLEAN="${arg#*=}"
    ;;
  *)
    echo "Unknown option: $arg"
    exit 1
    ;;
  esac
done


if [ -z $GAME_DIR ]; then
  GAME_DIR="./src/demo"
  ENGINE_DIR="./"
  OUTPUT_DIR="./build"
  CLEAN=1
fi

# -----------------------------
# Prepare output directory
# -----------------------------

if [ "$CLEAN" -eq 1 ]; then
  rm -rf "$OUTPUT_DIR"
fi

mkdir -p "$OUTPUT_DIR"

#CFLAGS="-Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wswitch-enum  -pedantic -fno-exceptions -fstack-protector -g -fsanitize=address"
CFLAGS="-Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wswitch-enum  -pedantic -fno-exceptions -fstack-protector -g"
CC="clang"

source "build_common.sh"

#export ASAN_OPTIONS=detect_stack_use_after_return=1
#export LSAN_OPTIONS=suppressions=lsan_ignore.txt
#export LSAN_OPTIONS=verbosity=1:log_threads=1

# -----------------------------
# Build the game shared library
# -----------------------------
echo "Building game..."

CFLAGS="${CFLAGS:-} -std=gnu23"
#@TODO: remove -lgame we NEED the reload ok?! only for release builds this bullshit
CLIBS="-lX11 -lGL -lGLEW -lXrandr -lm -lgame"

DEBUG_FLAGS="-O0 -g"
RELEASE_FLAGS="-O2"
INCLUDE_DIRS="-Iext -I$ENGINE_DIR/frz -I$ENGINE_DIR/src"

$CC $CFLAGS $DEBUG_FLAGS $INCLUDE_DIRS -fPIC -shared -lm \
"$GAME_DIR"/*.c \
"$ENGINE_DIR"/src/gui/*.c \
"$ENGINE_DIR"/src/gui2/*.c \
-o "$OUTPUT_DIR/libgame.so"

[ $? -eq 0 ] && echo "Game Build succeeded. ✅" || { echo "Game Build failed. ❌"; exit 1; }

# -----------------------------
# Build the engine executable
# -----------------------------
echo "Building engine..."

pushd "$ENGINE_DIR" > /dev/null

$CC $CFLAGS $DEBUG_FLAGS \
    -Iext -Isrc -I"$GAME_DIR" \
    -L"$OUTPUT_DIR" \
    src/core/*.c \
    src/platform/platform_rgfw.c \
    -o "$OUTPUT_DIR/prototype" \
    $CLIBS \
    -Wl,-rpath,'$ORIGIN'

popd > /dev/null

[ $? -eq 0 ] && echo "Core Build succeeded. ✅" || { echo "Core Build failed. ❌"; exit 1; }
