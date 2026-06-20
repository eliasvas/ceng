#!/usr/bin/env bash
set -e

# TODO: Maybe we could add the asset handling here
# TODO: Maybe compile with -pedantic
#CFLAGS="-Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wswitch-enum  -pedantic -fno-exceptions -fstack-protector -g -fsanitize=address"
CFLAGS="-Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wswitch-enum -fno-exceptions -fstack-protector -g"
CC="clang"

# -----------------------------
# Parse arguments
# Usage: ./build.sh ed=./ceng gd=../my_game od=out clean=0
# --------------------------------------------------------------

CLEAN=1

for arg in "$@"; do
  case $arg in
  gd=*)
    GAME_DIR="${arg#*=}"
    ;;
  od=*)
    OUTPUT_DIR="${arg#*=}"
    ;;
  ed=*)
    ENGINE_DIR="${arg#*=}"
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
# --------------------------------------------------------------
# Convert to realpaths
GAME_DIR="$(realpath "$GAME_DIR")"
OUTPUT_DIR="$(realpath "$OUTPUT_DIR")"
ENGINE_DIR="$(realpath "$ENGINE_DIR")"
EXT_DIR="./ext"

# -----------------------------
# Prepare output directory
# -----------------------------

if [ "$CLEAN" -eq 1 ]; then
  rm -rf "$OUTPUT_DIR"
fi

mkdir -p "$OUTPUT_DIR"

#export ASAN_OPTIONS=detect_stack_use_after_return=1
#export LSAN_OPTIONS=suppressions=lsan_ignore.txt
#export LSAN_OPTIONS=verbosity=1:log_threads=1

# -----------------------------
# Build the game shared library
# -----------------------------

CFLAGS="${CFLAGS:-} -std=gnu23"
#@TODO: remove -lgame we NEED the reload ok?! only for release builds this bullshit
CLIBS="-lX11 -lGL -lGLEW -lXrandr -lm -lgame"

DEBUG_FLAGS="-O0 -g"
RELEASE_FLAGS="-O2"
INCLUDE_DIRS="-Iext -I$ENGINE_DIR/frz -I$ENGINE_DIR/src -I$GAME_DIR"


start=$(date +%s.%3N)
echo "Building gamelib.."
$CC $CFLAGS $DEBUG_FLAGS $INCLUDE_DIRS -fPIC -shared -lm \
"$GAME_DIR"/*.c \
"$ENGINE_DIR"/src/gui/*.c \
"$ENGINE_DIR"/src/gui2/*.c \
-o "$OUTPUT_DIR/libgame.so"
elapsed=$(echo "$(date +%s.%3N) - $start" | bc)
[ $? -eq 0 ] && echo "done in ($elapsed) ✅" || { echo "failed ❌"; exit 1; }

# -----------------------------
# Build miniaudio
# -----------------------------
start=$(date +%s.%3N)
echo "Building miniaudio.."
$CC $DEBUG_FLAGS $INCLUDE_DIRS "$EXT_DIR/miniaudio/miniaudio.c" -shared -fPIC -lm -o "$OUTPUT_DIR/ma.o"
elapsed=$(echo "$(date +%s.%3N) - $start" | bc)
[ $? -eq 0 ] && echo "done in ($elapsed) ✅" || { echo "failed ❌"; exit 1; }

# -----------------------------
# Build the engine executable
# -----------------------------
start=$(date +%s.%3N)
echo "Building engine..."
pushd "$ENGINE_DIR" > /dev/null
$CC $CFLAGS $DEBUG_FLAGS \
    -Iext -Isrc -I$GAME_DIR \
    -L"$OUTPUT_DIR" \
    src/core/*.c \
    src/platform/platform_rgfw.c \
    "$OUTPUT_DIR/ma.o" \
    -o "$OUTPUT_DIR/ceng" \
    $CLIBS \
    -Wl,-rpath,'$ORIGIN'
popd > /dev/null
elapsed=$(echo "$(date +%s.%3N) - $start" | bc)
[ $? -eq 0 ] && echo "done in ($elapsed) ✅" || { echo "failed ❌"; exit 1; }
