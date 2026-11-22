#!/usr/bin/env bash

set -euo pipefail

readonly ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
readonly SCRIPT_PATH="${ROOT_DIR}/$(basename -- "${BASH_SOURCE[0]}")"
readonly CACHE_ROOT="${ROOT_DIR}/.cache"
readonly DEPS_ROOT="${CACHE_ROOT}/deps"
readonly DOWNLOAD_ROOT="${CACHE_ROOT}/downloads"
readonly YAML_CPP_SOURCE="${ROOT_DIR}/3rdparty/yaml-cpp"
readonly CPR_SOURCE="${ROOT_DIR}/3rdparty/cpr"
readonly PORTAUDIO_SOURCE="${ROOT_DIR}/3rdparty/portaudio"
readonly GENERATOR="${UFEEL_GENERATOR:-Ninja}"
readonly C_COMPILER="${UFEEL_C_COMPILER:-clang-22}"
readonly CXX_COMPILER="${UFEEL_CXX_COMPILER:-clang++-22}"

export CCACHE_DIR="${CCACHE_DIR:-$CACHE_ROOT/ccache}"

usage()
{
    cat <<'EOF'
Usage: ./build-deps.sh [--print-prefix]

Builds the 3rdparty submodules (yaml-cpp, cpr with its built-in curl, PortAudio with ALSA and PulseAudio) once into .cache/deps/<key> and prints that prefix. The key covers the submodule sources, the compilers and this script, so nothing is rebuilt while they are unchanged.

Options:
  --print-prefix   Print the prefix without building
  -h, --help       Show this help

Environment:
  UFEEL_C_COMPILER, UFEEL_CXX_COMPILER, UFEEL_GENERATOR, CCACHE_DIR
EOF
}

error()
{
    printf 'error: %s\n' "$*" >&2
    exit 1
}

run()
{
    printf '+' >&2
    printf ' %q' "$@" >&2
    printf '\n' >&2
    "$@" >&2
}

compiler_launcher_options()
{
    command -v ccache >/dev/null 2>&1 || return 0
    printf '%s\n' -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
}

download()
{
    local url="$1"
    local destination="$2"
    if command -v curl >/dev/null 2>&1; then
        run curl -fsSL --retry 3 -o "$destination" "$url"
    elif command -v wget >/dev/null 2>&1; then
        run wget -q -O "$destination" "$url"
    else
        error "curl or wget is required to download $url"
    fi
}

curl_source()
{
    local declaration url sha256 archive source
    declaration="$(sed -n '/FetchContent_Declare(curl/,/)/p' "$CPR_SOURCE/CMakeLists.txt")"
    url="$(grep -oP 'URL \K\S+' <<<"$declaration" | head -n 1)"
    sha256="$(grep -oP 'SHA256=\K[0-9a-f]{64}' <<<"$declaration")"
    [[ -n "$url" && -n "$sha256" ]] ||
        error "cannot read the curl URL and SHA-256 from $CPR_SOURCE/CMakeLists.txt"
    archive="$DOWNLOAD_ROOT/${url##*/}"
    source="$DOWNLOAD_ROOT/curl-${sha256:0:16}"
    if [[ ! -f "$source/CMakeLists.txt" ]]; then
        mkdir -p "$DOWNLOAD_ROOT"
        if [[ ! -f "$archive" ]] || ! sha256sum --check --quiet <<<"$sha256  $archive" 2>/dev/null; then
            download "$url" "$archive.part"
            sha256sum --check --quiet <<<"$sha256  $archive.part" ||
                error "SHA-256 mismatch for $url"
            mv "$archive.part" "$archive"
        fi
        rm -rf "$source.part"
        mkdir -p "$source.part"
        (cd "$source.part" && cmake -E tar xf "$archive")
        mv "$source.part"/*/ "$source"
        rm -rf "$source.part"
    fi
    printf '%s' "$source"
}

source_state()
{
    local directory="$1"
    [[ -f "$directory/CMakeLists.txt" ]] ||
        error "$directory is missing, run 'git submodule update --init --recursive'"
    if [[ "$(git -C "$directory" rev-parse --show-toplevel 2>/dev/null)" == "$directory" ]]; then
        git -C "$directory" rev-parse HEAD
        git -C "$directory" status --porcelain --untracked-files=no | sha256sum
    else
        find "$directory" -type f ! -path '*/.git/*' -print0 | sort -z | xargs -0 sha256sum | sha256sum
    fi
}

dependencies_prefix()
{
    local key
    key="$(
        {
            source_state "$YAML_CPP_SOURCE"
            source_state "$CPR_SOURCE"
            source_state "$PORTAUDIO_SOURCE"
            "$C_COMPILER" --version | head -n 1
            "$CXX_COMPILER" --version | head -n 1
            printf '%s\n' "$GENERATOR"
            sha256sum < "$SCRIPT_PATH"
        } | sha256sum | cut -c1-16
    )"
    printf '%s/%s' "$DEPS_ROOT" "$key"
}

build_yaml_cpp()
{
    local build_directory="$1"
    local prefix="$2"
    shift 2
    run cmake -S "$YAML_CPP_SOURCE" -B "$build_directory/yaml-cpp" "$@" \
        -DYAML_BUILD_SHARED_LIBS=ON \
        -DYAML_CPP_BUILD_TESTS=OFF \
        -DYAML_CPP_BUILD_TOOLS=OFF \
        -DYAML_CPP_BUILD_CONTRIB=OFF \
        -DYAML_CPP_FORMAT_SOURCE=OFF
    run cmake --build "$build_directory/yaml-cpp" --parallel
    run cmake --install "$build_directory/yaml-cpp" --prefix "$prefix"
}

build_cpr()
{
    local build_directory="$1"
    local prefix="$2"
    shift 2
    local curl_directory
    curl_directory="$(curl_source)"
    run cmake -S "$CPR_SOURCE" -B "$build_directory/cpr" "$@" \
        -DBUILD_SHARED_LIBS=OFF \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        "-DFETCHCONTENT_SOURCE_DIR_CURL=$curl_directory" \
        -DFETCHCONTENT_FULLY_DISCONNECTED=ON \
        -DCPR_BUILD_TESTS=OFF \
        -DCPR_ENABLE_INSTALL=ON \
        -DCPR_ENABLE_SSL=OFF \
        -DCPR_USE_SYSTEM_CURL=OFF \
        -DCPR_ENABLE_CURL_HTTP_ONLY=ON \
        -DCPR_CURL_USE_LIBPSL=OFF \
        -DCURL_ZLIB=OFF \
        -DCURL_BROTLI=OFF \
        -DCURL_ZSTD=OFF \
        -DUSE_NGHTTP2=OFF \
        -DUSE_LIBIDN2=OFF \
        -DCURL_USE_LIBSSH2=OFF \
        -DCURL_USE_LIBSSH=OFF \
        -DCURL_USE_GSSAPI=OFF \
        -DBUILD_LIBCURL_DOCS=OFF \
        -DBUILD_MISC_DOCS=OFF \
        -DENABLE_CURL_MANUAL=OFF
    run cmake --build "$build_directory/cpr" --parallel
    run cmake --install "$build_directory/cpr" --prefix "$prefix"
}

build_portaudio()
{
    local build_directory="$1"
    local prefix="$2"
    shift 2
    run cmake -S "$PORTAUDIO_SOURCE" -B "$build_directory/portaudio" "$@" \
        -DPA_BUILD_SHARED_LIBS=ON \
        -DPA_USE_ALSA=ON \
        -DPA_USE_PULSEAUDIO=ON \
        -DPA_USE_JACK=OFF \
        -DPA_USE_OSS=OFF \
        -DPA_USE_SNDIO=OFF
    grep -q '^PA_USE_PULSEAUDIO:BOOL=ON$' "$build_directory/portaudio/CMakeCache.txt" &&
        grep -q '^PA_USE_ALSA:BOOL=ON$' "$build_directory/portaudio/CMakeCache.txt" ||
        error "PortAudio needs the ALSA and PulseAudio development files (libasound2-dev, libpulse-dev)"
    run cmake --build "$build_directory/portaudio" --parallel
    run cmake --install "$build_directory/portaudio" --prefix "$prefix"
}

build_dependencies()
{
    local prefix="$1"
    if [[ -f "$prefix/.complete" ]]; then
        printf 'Dependencies up to date in %s\n' "$prefix" >&2
        return
    fi
    local build_directory="$prefix.build"
    rm -rf "$prefix" "$build_directory" ||
        error "cannot remove $prefix, check its ownership and permissions"
    local -a options=(
        -G "$GENERATOR"
        -DCMAKE_BUILD_TYPE=Release
        "-DCMAKE_C_COMPILER=$C_COMPILER"
        "-DCMAKE_CXX_COMPILER=$CXX_COMPILER"
        -DCMAKE_INSTALL_LIBDIR=lib
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5
    )
    mapfile -t -O "${#options[@]}" options < <(compiler_launcher_options)
    build_yaml_cpp "$build_directory" "$prefix" "${options[@]}"
    build_cpr "$build_directory" "$prefix" "${options[@]}"
    build_portaudio "$build_directory" "$prefix" "${options[@]}"
    touch "$prefix/.complete"
    rm -rf "$build_directory"
}

print_only=false
while (($#)); do
    case "$1" in
        --print-prefix) print_only=true ;;
        -h|--help) usage; exit 0 ;;
        *) usage >&2; error "unknown option '$1'" ;;
    esac
    shift
done

command -v cmake >/dev/null 2>&1 || error "required command 'cmake' is not installed"
prefix="$(dependencies_prefix)"
"$print_only" || build_dependencies "$prefix"
printf '%s\n' "$prefix"
