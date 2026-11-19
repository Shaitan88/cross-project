#!/usr/bin/env bash
#
# Build Castle Escape for the browser with Emscripten/WebAssembly.
#
# Everything (emsdk, Lua 5.4.8) is installed under the user's home or below
# build-web; no packages are installed and no sudo is required.
#
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd)"

BUILD="$ROOT/build-web"
DOWNLOADS="$BUILD/downloads"
SRC="$BUILD/src"
LUA_VERSION="5.4.8"
LUA_TARBALL="lua-$LUA_VERSION.tar.gz"
LUA_PRIMARY="https://www.lua.org/ftp/$LUA_TARBALL"
LUA_FALLBACK="https://github.com/lua/lua/archive/refs/tags/v$LUA_VERSION.tar.gz"
LUA_SRC="$SRC/lua-$LUA_VERSION"
LUA_BUILD="$BUILD/lua"
PROJECT_BUILD="$BUILD/project"
DIST="$ROOT/dist/web"
EMSDK_DIR="${EMSDK_DIR:-$HOME/.local/emsdk}"
EMSDK_VERSION="${EMSDK_VERSION:-7.0.0}"

mkdir -p "$DOWNLOADS" "$SRC" "$LUA_BUILD" "$DIST"

log() { printf '%s\n' "==> $*"; }

require() {
    command -v "$1" >/dev/null 2>&1 || {
        echo "ERROR: required command not found: $1" >&2
        exit 1
    }
}

# --- emsdk ----------------------------------------------------------------
setup_emsdk() {
    if command -v emcc >/dev/null 2>&1; then
        return
    fi
    if [ -f "$EMSDK_DIR/emsdk_env.sh" ]; then
        # shellcheck disable=SC1090,SC1091
        source "$EMSDK_DIR/emsdk_env.sh" >/dev/null 2>&1 || true
        if command -v emcc >/dev/null 2>&1; then
            return
        fi
    fi

    local emcc="$EMSDK_DIR/upstream/emscripten/emcc"
    if [ ! -x "$emcc" ]; then
        if pgrep -af 'emsdk(_env)?\.sh|emsdk\.py.*install' >/dev/null 2>&1; then
            log "An emsdk install is already running; waiting for $emcc"
            for _ in $(seq 1 120); do
                [ -x "$emcc" ] && break
                sleep 5
            done
        else
            log "Installing emsdk into $EMSDK_DIR"
            if [ ! -x "$EMSDK_DIR/emsdk" ]; then
                require git
                git clone https://github.com/emscripten-core/emsdk.git "$EMSDK_DIR"
            fi
            ( cd "$EMSDK_DIR" && ./emsdk install "$EMSDK_VERSION" && ./emsdk activate "$EMSDK_VERSION" )
        fi
    fi

    if [ ! -x "$emcc" ]; then
        log "Waiting for emsdk to finish activating"
        for _ in $(seq 1 60); do
            [ -x "$emcc" ] && break
            sleep 5
        done
    fi

    if [ -f "$EMSDK_DIR/emsdk_env.sh" ]; then
        # shellcheck disable=SC1090,SC1091
        source "$EMSDK_DIR/emsdk_env.sh" >/dev/null 2>&1 || true
    fi
    command -v emcc >/dev/null 2>&1 || {
        echo "ERROR: emcc is unavailable; emsdk install did not complete" >&2
        exit 1
    }
}

# --- Lua 5.4.8 ------------------------------------------------------------
fetch() {
    local url="$1" dest="$2"
    if [ -s "$dest" ]; then
        return 0
    fi
    local tmp="$dest.part"
    local attempt
    for attempt in 1 2 3 4 5; do
        log "Downloading $url (attempt $attempt)"
        if curl -fL --retry 3 --retry-delay 2 -o "$tmp" "$url"; then
            mv "$tmp" "$dest"
            return 0
        fi
        sleep 2
    done
    rm -f "$tmp"
    return 1
}

obtain_lua_source() {
    if [ -d "$LUA_SRC/src" ]; then
        return
    fi
    if [ ! -s "$DOWNLOADS/$LUA_TARBALL" ]; then
        if ! fetch "$LUA_PRIMARY" "$DOWNLOADS/$LUA_TARBALL"; then
            log "Primary Lua download failed; trying the GitHub mirror"
            fetch "$LUA_FALLBACK" "$DOWNLOADS/$LUA_TARBALL"
        fi
    fi
    log "Extracting $LUA_TARBALL"
    tar -xzf "$DOWNLOADS/$LUA_TARBALL" -C "$SRC"
    # The GitHub mirror extracts the C sources at the archive root; normalize
    # it so both layouts expose $LUA_SRC/src.
    if [ ! -d "$LUA_SRC/src" ] && [ -f "$LUA_SRC/lua.c" ]; then
        mkdir -p "$LUA_SRC/src"
        mv "$LUA_SRC"/*.c "$LUA_SRC"/*.h "$LUA_SRC/src/" 2>/dev/null || true
    fi
    [ -d "$LUA_SRC/src" ] || {
        echo "ERROR: Lua source layout not recognized under $LUA_SRC" >&2
        exit 1
    }
}

build_lua() {
    local lib="$LUA_BUILD/liblua.a"
    if [ -f "$lib" ]; then
        log "Reusing $lib"
        return
    fi
    local objdir="$LUA_BUILD/obj"
    mkdir -p "$objdir"
    rm -f "$objdir"/*.o
    local objects=()
    local source
    for source in "$LUA_SRC/src"/*.c; do
        case "$(basename "$source")" in
            lua.c|luac.c) continue ;;
        esac
        local obj="$objdir/$(basename "${source%.c}").o"
        emcc -O2 -I"$LUA_SRC/src" -c "$source" -o "$obj"
        objects+=("$obj")
    done
    [ "${#objects[@]}" -gt 0 ] || {
        echo "ERROR: no Lua sources compiled" >&2
        exit 1
    }
    log "Archiving $lib"
    emar rcs "$lib" "${objects[@]}"
}

wait_for_shell() {
    local shell="$ROOT/web/shell.html"
    if [ -f "$shell" ]; then
        return
    fi
    log "Waiting for web/shell.html (owned by another agent)"
    for _ in $(seq 1 60); do
        [ -f "$shell" ] && return
        sleep 5
    done
    echo "ERROR: web/shell.html is still missing after 5 minutes" >&2
    exit 1
}

# --- main -----------------------------------------------------------------
log "Castle Escape: WebAssembly build"
log "Repository: $ROOT"

setup_emsdk
log "Using $(emcc --version | head -n 1)"

obtain_lua_source
build_lua
wait_for_shell

log "Configuring project"
emcmake cmake -S "$ROOT" -B "$PROJECT_BUILD" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=OFF \
    -DLUA_INCLUDE_DIR="$LUA_SRC/src" \
    -DLUA_LIBRARY="$LUA_BUILD/liblua.a"

log "Building project"
cmake --build "$PROJECT_BUILD" --parallel

for ext in html js wasm; do
    artifact="$DIST/castle_escape.$ext"
    [ -f "$artifact" ] || {
        echo "ERROR: expected artifact missing: $artifact" >&2
        exit 1
    }
done

log "Web artifacts:"
ls -lh "$DIST/castle_escape.html" "$DIST/castle_escape.js" "$DIST/castle_escape.wasm"
