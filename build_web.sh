# Good reference for emscripten building https://github.com/ocornut/imgui/blob/master/examples/example_sdl3_opengl3/Makefile.emscripten
# TODO: MUCH more customization needed, rn its GARBO

# source "build_common.sh" ------------------------------------------

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
# -----------------------------------------------------------------------

EMCC=emcc

CFLAGS="${CFLAGS:-} -std=gnu23"
OPT_FLAGS="${OPT_FLAGS:- -O1}"

INCLUDES="-Iext -Isrc -Isrc/demo"

EM_FLAGS="\
-s DISABLE_EXCEPTION_CATCHING=1 \
-s WASM=1 \
-s ALLOW_MEMORY_GROWTH=1 \
-s NO_EXIT_RUNTIME=0 \
-s ASSERTIONS=2 \
-s USE_WEBGL2=1 \
-s MIN_WEBGL_VERSION=2 \
-s MAX_WEBGL_VERSION=2 \
-s FULL_ES3=1 \
-s OFFSCREENCANVAS_SUPPORT \
-s ASYNCIFY \
-s ASYNCIFY_STACK_SIZE=65536 \
-s STACK_SIZE=1048576 \
-s SAFE_HEAP=1 \
-s GL_DEBUG=1"


LDFLAGS="${LDFLAGS:-}"

echo "Building WASM game..."

EMCC_DEBUG=1 $EMCC -v \
  $CFLAGS $OPT_FLAGS $INCLUDES \
  -fPIC -shared \
  "$GAME_DIR"/*.c src/gui/*.c \
  -o build/libgame.so \
  $EM_FLAGS \
&& echo "WASM Game Build succeeded. ✅" \
|| { echo "WASM Game Build failed. ❌"; exit 1; }

EMCC_DEBUG=1 $EMCC -v -g \
  $CFLAGS $OPT_FLAGS $INCLUDES \
  -Lbuild \
  src/core/*.c \
  src/platform/platform_rgfw.c \
  -lgame \
  -o build/index.html \
  -Wl,-rpath='build/' \
  $EM_FLAGS \
  --preload-file data \
  $LDFLAGS \
&& echo "WASM Core Build succeeded. ✅" \
|| { echo "WASM Core Build failed. ❌"; exit 1; }

