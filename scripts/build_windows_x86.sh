#!/usr/bin/env bash
#
# Build Castle Escape for 32-bit Windows (x86) with MinGW-w64, then run the
# test suite and the SDL smoke test under Wine.
#
# Everything is downloaded and built below BUILD_DIR; no packages are
# installed and no sudo is required.
#
# Environment overrides:
#   BUILD_DIR   build root (default: <repo>/build-windows-x86)
#
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd)"

BUILD_DIR="${BUILD_DIR:-$REPO_ROOT/build-windows-x86}"
case "$BUILD_DIR" in
    /*) ;;
    *) BUILD_DIR="$REPO_ROOT/$BUILD_DIR" ;;
esac

TOOLCHAIN="$REPO_ROOT/cmake/toolchains/mingw-w64-i686.cmake"

LUA_VERSION="5.4.8"
SDL_VERSION="2.32.10"

DOWNLOADS="$BUILD_DIR/downloads"
SRC="$BUILD_DIR/src"
DEPS="$BUILD_DIR/deps/i686"
SDL_BUILD="$BUILD_DIR/sdl2-build"
PROJECT_BUILD="$BUILD_DIR/project"
DIST="$REPO_ROOT/dist/windows-x86"
DIST_EXE="$DIST/castle_escape.exe"
LOGS="$BUILD_DIR/logs"
WINEPREFIX_DIR="$BUILD_DIR/wineprefix"

mkdir -p "$DOWNLOADS" "$SRC" "$DEPS/lib" "$DEPS/include" \
         "$DIST" "$LOGS"

echo "== Castle Escape: Windows x86 build =="
echo "Repository: $REPO_ROOT"
echo "Build dir:  $BUILD_DIR"
echo "Toolchain:  $TOOLCHAIN"

require() {
    command -v "$1" >/dev/null 2>&1 || {
        echo "ERROR: required command not found: $1" >&2
        exit 1
    }
}

for tool in \
    i686-w64-mingw32-g++ i686-w64-mingw32-gcc i686-w64-mingw32-ar \
    i686-w64-mingw32-ranlib i686-w64-mingw32-strip i686-w64-mingw32-windres \
    i686-w64-mingw32-objdump wine wineboot cmake curl make tar file; do
    require "$tool"
done

download() {
    local name="$1"
    shift
    local out="$DOWNLOADS/$name"
    if [ -s "$out" ]; then
        echo "Cached download: $out"
        return 0
    fi
    local url
    for url in "$@"; do
        echo "Downloading $url"
        if curl -fL --retry 3 --retry-delay 2 --connect-timeout 20 \
                -o "$out.part" "$url"; then
            mv "$out.part" "$out"
            return 0
        fi
        rm -f "$out.part"
        echo "WARN: download failed, trying next mirror: $url" >&2
    done
    echo "ERROR: could not download $name" >&2
    return 1
}

download "lua-$LUA_VERSION.tar.gz" \
    "https://www.lua.org/ftp/lua-$LUA_VERSION.tar.gz" \
    "https://github.com/lua/lua/archive/refs/tags/v$LUA_VERSION.tar.gz"

download "SDL2-$SDL_VERSION.tar.gz" \
    "https://libsdl.org/release/SDL2-$SDL_VERSION.tar.gz" \
    "https://github.com/libsdl-org/SDL/releases/download/release-$SDL_VERSION/SDL2-$SDL_VERSION.tar.gz"

LUA_SRC="$SRC/lua-$LUA_VERSION"
if [ ! -f "$LUA_SRC/src/lua.h" ]; then
    echo "Extracting Lua $LUA_VERSION"
    tar -xzf "$DOWNLOADS/lua-$LUA_VERSION.tar.gz" -C "$SRC"
fi
if [ ! -f "$LUA_SRC/src/lua.h" ]; then
    echo "ERROR: Lua source tree not found at $LUA_SRC" >&2
    exit 1
fi

SDL_SRC="$SRC/SDL2-$SDL_VERSION"
if [ ! -f "$SDL_SRC/CMakeLists.txt" ]; then
    echo "Extracting SDL2 $SDL_VERSION"
    tar -xzf "$DOWNLOADS/SDL2-$SDL_VERSION.tar.gz" -C "$SRC"
fi
# Accept a GitHub source archive name as well.
if [ ! -f "$SDL_SRC/CMakeLists.txt" ] && [ -f "$SRC/SDL-release-$SDL_VERSION/CMakeLists.txt" ]; then
    mv "$SRC/SDL-release-$SDL_VERSION" "$SDL_SRC"
fi
if [ ! -f "$SDL_SRC/CMakeLists.txt" ]; then
    echo "ERROR: SDL2 source tree not found at $SDL_SRC" >&2
    exit 1
fi

# ---------------------------------------------------------------------------
# Lua 5.4 static library for i686-w64-mingw32
# ---------------------------------------------------------------------------
echo "== Building Lua $LUA_VERSION (static, i686-w64-mingw32) =="
make -C "$LUA_SRC/src" a \
    CC="i686-w64-mingw32-gcc -std=gnu99" \
    AR="i686-w64-mingw32-ar rcu" \
    RANLIB="i686-w64-mingw32-ranlib" \
    MYCFLAGS="-DLUA_USE_WINDOWS"

cp -f "$LUA_SRC/src/liblua.a" "$DEPS/lib/liblua.a"
for header in lua.h luaconf.h lualib.h lauxlib.h lua.hpp; do
    cp -f "$LUA_SRC/src/$header" "$DEPS/include/$header"
done
i686-w64-mingw32-ranlib "$DEPS/lib/liblua.a"
echo "Installed $DEPS/lib/liblua.a"

# ---------------------------------------------------------------------------
# SDL2 static library for i686-w64-mingw32
# ---------------------------------------------------------------------------
echo "== Building SDL2 $SDL_VERSION (static, i686-w64-mingw32) =="
cmake -S "$SDL_SRC" -B "$SDL_BUILD" \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
    -DCMAKE_INSTALL_PREFIX="$DEPS" \
    -DCMAKE_BUILD_TYPE=Release \
    -DSDL_SHARED=OFF \
    -DSDL_STATIC=ON \
    -DSDL_TEST=OFF \
    -DSDL_TESTS=OFF \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build "$SDL_BUILD" --parallel
cmake --install "$SDL_BUILD"

if [ ! -f "$DEPS/lib/liblua.a" ]; then
    echo "ERROR: liblua.a was not installed" >&2
    exit 1
fi
if [ ! -f "$DEPS/lib/libSDL2.a" ]; then
    echo "ERROR: static SDL2 library was not installed" >&2
    exit 1
fi

# ---------------------------------------------------------------------------
# Project
# ---------------------------------------------------------------------------
echo "== Configuring Castle Escape for Windows x86 =="
FIND_ROOT_PATH="/usr/i686-w64-mingw32;$DEPS"
cmake -S "$REPO_ROOT" -B "$PROJECT_BUILD" \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$DEPS" \
    -DSDL2_DIR="$DEPS/lib/cmake/SDL2" \
    -DLUA_INCLUDE_DIR="$DEPS/include" \
    -DLUA_LIBRARY="$DEPS/lib/liblua.a" \
    -DBUILD_TESTING=ON \
    -DCMAKE_EXE_LINKER_FLAGS="-static -static-libgcc -static-libstdc++" \
    -DCMAKE_CROSSCOMPILING_EMULATOR=wine \
    -DCMAKE_FIND_ROOT_PATH="$FIND_ROOT_PATH"

echo "== Building Castle Escape for Windows x86 =="
cmake --build "$PROJECT_BUILD" --parallel

if [ ! -f "$PROJECT_BUILD/castle_escape.exe" ]; then
    echo "ERROR: castle_escape.exe was not produced" >&2
    exit 1
fi
cp -f "$PROJECT_BUILD/castle_escape.exe" "$DIST_EXE"

# ---------------------------------------------------------------------------
# Artifact verification
# ---------------------------------------------------------------------------
echo "== Verifying artifact =="
FILE_OUT="$(file "$DIST_EXE")"
echo "$FILE_OUT"
if ! grep -Eq 'PE32 executable.*(Intel 80386|Intel i386)' <<<"$FILE_OUT"; then
    echo "ERROR: artifact is not a PE32 Intel 80386 executable" >&2
    exit 1
fi

DEPS_OUT="$(i686-w64-mingw32-objdump -p "$DIST_EXE" | grep -i 'DLL Name:' || true)"
echo "DLL dependencies:"
echo "$DEPS_OUT"
if grep -Eiq 'DLL Name: (libstdc\+\+-6|libgcc_s[^ ]*|libwinpthread-1|SDL2|lua[^ ]*)\.dll' <<<"$DEPS_OUT"; then
    echo "ERROR: artifact depends on a DLL that must be statically linked" >&2
    exit 1
fi

# ---------------------------------------------------------------------------
# Wine test run
# ---------------------------------------------------------------------------
export WINEPREFIX="$WINEPREFIX_DIR"
export WINEDEBUG=-all
unset WINEARCH || true
# Dummy SDL backends are needed by the SDL smoke test in a headless environment.
export SDL_VIDEODRIVER=dummy
export SDL_AUDIODRIVER=dummy

if [ ! -d "$WINEPREFIX" ]; then
    echo "== Initializing Wine prefix at $WINEPREFIX =="
    if ! wineboot -u >"$LOGS/wineboot.log" 2>&1; then
        echo "ERROR: Wine prefix initialization failed" >&2
        cat "$LOGS/wineboot.log" >&2
        exit 1
    fi
fi

echo "== Running the six tests under Wine (ctest) =="
CTEST_LOG="$LOGS/ctest.log"
set +e
ctest --test-dir "$PROJECT_BUILD" --output-on-failure 2>&1 | tee "$CTEST_LOG"
CTEST_RC=${PIPESTATUS[0]}
set -e

run_manual_tests() {
    local failed=0
    local exe
    for exe in game_logic_test guards_rooms_test combat_alarm_test \
               objectives_test application_test; do
        echo "--- wine $exe.exe ---"
        if ! wine "$PROJECT_BUILD/$exe.exe" >"$LOGS/manual-$exe.log" 2>&1; then
            failed=1
        fi
        cat "$LOGS/manual-$exe.log"
    done
    echo "--- wine castle_escape.exe --smoke-test ---"
    if ! wine "$PROJECT_BUILD/castle_escape.exe" --smoke-test \
            >"$LOGS/manual-castle_escape.log" 2>&1; then
        failed=1
    fi
    cat "$LOGS/manual-castle_escape.log"
    return "$failed"
}

if [ "$CTEST_RC" -ne 0 ]; then
    echo "WARN: ctest exited $CTEST_RC; re-running all six tests manually under Wine" >&2
    if ! run_manual_tests; then
        echo "ERROR: Windows x86 tests failed" >&2
        exit 1
    fi
fi

echo "== Explicit SDL smoke test under Wine =="
SMOKE_LOG="$LOGS/smoke.log"
wine "$PROJECT_BUILD/castle_escape.exe" --smoke-test >"$SMOKE_LOG" 2>&1
cat "$SMOKE_LOG"
SMOKE_OUT="$(cat "$SMOKE_LOG")"

for needle in 'PASS:' 'player x=304' 'Ammo 5' 'Quiet'; do
    if ! grep -q "$needle" <<<"$SMOKE_OUT"; then
        echo "ERROR: smoke output is missing: $needle" >&2
        exit 1
    fi
done

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
echo
echo "================ Summary ================"
echo "Artifact:     $DIST_EXE"
echo "file:         $FILE_OUT"
echo "DLL deps:"
echo "$DEPS_OUT"
if [ "$CTEST_RC" -eq 0 ]; then
    echo "ctest:        PASS (exit 0)"
else
    echo "ctest:        exit $CTEST_RC; manual Wine run PASS"
fi
echo "smoke:        $(tr '\n' ' ' <<<"$SMOKE_OUT")"
echo "========================================="
echo "Windows x86 build and Wine checks passed."
