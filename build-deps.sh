#!/usr/bin/env bash

set -euo pipefail

if ((BASH_VERSINFO[0] * 100 + BASH_VERSINFO[1] < 404)); then
    printf 'error: bash 4.4 or newer is required, found %s (on macOS: brew install bash)\n' \
        "$BASH_VERSION" >&2
    exit 1
fi

# Fail on errors inside $(...) too, where most of the building happens.
shopt -s inherit_errexit

readonly ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
readonly SCRIPT_PATH="${ROOT_DIR}/$(basename -- "${BASH_SOURCE[0]}")"
readonly CACHE_ROOT="${ROOT_DIR}/.cache"
readonly DEPS_ROOT="${CACHE_ROOT}/deps"
readonly DOWNLOAD_ROOT="${CACHE_ROOT}/downloads"
readonly YAML_CPP_SOURCE="${ROOT_DIR}/3rdparty/yaml-cpp"
readonly CPR_SOURCE="${ROOT_DIR}/3rdparty/cpr"
readonly PORTAUDIO_SOURCE="${ROOT_DIR}/3rdparty/portaudio"
readonly PLATFORM="$(uname -s)"
readonly GENERATOR="${UFEEL_GENERATOR:-Ninja}"
readonly MACOS_DEPLOYMENT_TARGET="${UFEEL_MACOS_DEPLOYMENT_TARGET:-14.0}"

if [[ "$PLATFORM" == Darwin ]]; then
    readonly C_COMPILER="${UFEEL_C_COMPILER:-clang}"
    readonly CXX_COMPILER="${UFEEL_CXX_COMPILER:-clang++}"
else
    readonly C_COMPILER="${UFEEL_C_COMPILER:-clang-22}"
    readonly CXX_COMPILER="${UFEEL_CXX_COMPILER:-clang++-22}"
fi

# macOS has no prebuilt libraries in 3rdparty/: these are downloaded or built from source, then
# linked into the dependencies prefix under the same names as the 3rdparty/ directories.
readonly LIBTORCH_VERSION="2.10.0"
readonly LIBTORCH_URL="https://download.pytorch.org/libtorch/cpu/libtorch-macos-arm64-${LIBTORCH_VERSION}.zip"
readonly LIBTORCH_SHA256="13061d97e561235d9e43c132f6dc9b08a60f487a833da53a951614a96d3e7d32"
readonly OPENCV_VERSION="4.8.1"
readonly OPENCV_URL="https://github.com/opencv/opencv/archive/refs/tags/${OPENCV_VERSION}.tar.gz"
readonly OPENCV_SHA256="62f650467a60a38794d681ae7e66e3e8cfba38f445e0bf87867e2f2cdc8be9d5"
readonly TFLITE_VERSION="2.15.0"
readonly TFLITE_URL="https://github.com/tensorflow/tensorflow/archive/refs/tags/v${TFLITE_VERSION}.tar.gz"
readonly TFLITE_SHA256="9cec5acb0ecf2d47b16891f8bc5bc6fbfdffe1700bdadc0d9ebe27ea34f0c220"
readonly VOSK_VERSION="0.3.42"
readonly VOSK_URL="https://github.com/alphacep/vosk-api/releases/download/v${VOSK_VERSION}/vosk-osx-${VOSK_VERSION}.zip"
readonly VOSK_SHA256="65395f196c9d0583d79949142b25560acaf9c295f36284e18433097f3adb0ea1"

export CCACHE_DIR="${CCACHE_DIR:-$CACHE_ROOT/ccache}"

usage()
{
    cat <<'EOF'
Usage: ./build-deps.sh [--print-prefix]

Builds the 3rdparty submodules (yaml-cpp, cpr with its built-in curl, PortAudio with ALSA and PulseAudio, or CoreAudio on macOS) once into .cache/deps/<key> and prints that prefix. The key covers the submodule sources, the compilers and this script, so nothing is rebuilt while they are unchanged.

On macOS, the prefix also links LibTorch, OpenCV, TensorFlow Lite and Vosk, which are downloaded or built from source once into .cache/deps.

Options:
  --print-prefix   Print the prefix without building
  -h, --help       Show this help

Environment:
  UFEEL_C_COMPILER, UFEEL_CXX_COMPILER, UFEEL_GENERATOR, CCACHE_DIR,
  UFEEL_MACOS_DEPLOYMENT_TARGET (default: 14.0)
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

if command -v sha256sum >/dev/null 2>&1; then
    readonly SHA256_COMMAND=(sha256sum)
else
    readonly SHA256_COMMAND=(shasum -a 256)
fi

sha256()
{
    "${SHA256_COMMAND[@]}" "$@"
}

has_sha256()
{
    local file="$1"
    local expected="$2"
    [[ -f "$file" && "$(sha256 < "$file" | cut -d ' ' -f 1)" == "$expected" ]]
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

fetch()
{
    local url="$1"
    local sha256="$2"
    local archive="$DOWNLOAD_ROOT/$3"
    if ! has_sha256 "$archive" "$sha256"; then
        mkdir -p "$DOWNLOAD_ROOT"
        download "$url" "$archive.part"
        has_sha256 "$archive.part" "$sha256" || error "SHA-256 mismatch for $url"
        mv "$archive.part" "$archive"
    fi
    printf '%s' "$archive"
}

extract()
{
    local archive="$1"
    local destination="$2"
    local -a entries
    rm -rf "$destination" "$destination.part"
    mkdir -p "$destination.part"
    (cd "$destination.part" && cmake -E tar xf "$archive")
    entries=("$destination.part"/*)
    ((${#entries[@]} == 1)) && [[ -d "${entries[0]}" ]] ||
        error "$archive does not contain a single top-level directory"
    mv "${entries[0]}" "$destination"
    rm -rf "$destination.part"
}

curl_source()
{
    local declaration url sha256 archive source
    declaration="$(sed -n '/FetchContent_Declare(curl/,/)/p' "$CPR_SOURCE/CMakeLists.txt")"
    url="$(sed -n 's/.*URL[[:space:]]\{1,\}\([^[:space:]]*\).*/\1/p' <<<"$declaration" | head -n 1)"
    sha256="$(sed -n 's/.*SHA256=\([0-9a-f]\{64\}\).*/\1/p' <<<"$declaration")"
    [[ -n "$url" && -n "$sha256" ]] ||
        error "cannot read the curl URL and SHA-256 from $CPR_SOURCE/CMakeLists.txt"
    source="$DOWNLOAD_ROOT/curl-${sha256:0:16}"
    if [[ ! -f "$source/CMakeLists.txt" ]]; then
        archive="$(fetch "$url" "$sha256" "${url##*/}")"
        extract "$archive" "$source"
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
        git -C "$directory" status --porcelain --untracked-files=no | sha256
    else
        find "$directory" -type f ! -path '*/.git/*' -print0 | sort -z |
            xargs -0 "${SHA256_COMMAND[@]}" | sha256
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
            printf '%s\n' "$GENERATOR" "$MACOS_DEPLOYMENT_TARGET"
            sha256 < "$SCRIPT_PATH"
        } | sha256 | cut -c1-16
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
    # PortAudio always builds CoreAudio on macOS.
    local -a host_apis=()
    if [[ "$PLATFORM" != Darwin ]]; then
        host_apis=(-DPA_USE_ALSA=ON -DPA_USE_PULSEAUDIO=ON)
    fi
    run cmake -S "$PORTAUDIO_SOURCE" -B "$build_directory/portaudio" "$@" \
        -DPA_BUILD_SHARED_LIBS=ON \
        "${host_apis[@]}" \
        -DPA_USE_JACK=OFF \
        -DPA_USE_OSS=OFF \
        -DPA_USE_SNDIO=OFF
    if [[ "$PLATFORM" != Darwin ]]; then
        grep -q '^PA_USE_PULSEAUDIO:BOOL=ON$' "$build_directory/portaudio/CMakeCache.txt" &&
            grep -q '^PA_USE_ALSA:BOOL=ON$' "$build_directory/portaudio/CMakeCache.txt" ||
            error "PortAudio needs the ALSA and PulseAudio development files (libasound2-dev, libpulse-dev)"
    fi
    run cmake --build "$build_directory/portaudio" --parallel
    run cmake --install "$build_directory/portaudio" --prefix "$prefix"
}

patch_source()
{
    local file="$1"
    local expression="$2"
    local patched
    patched="$(sed -E "$expression" "$file")"
    [[ "$patched" != "$(cat "$file")" ]] || error "$file no longer needs the patch '$expression'"
    printf '%s\n' "$patched" > "$file"
}

use_rpath_install_name()
{
    local library="$1"
    run install_name_tool -id "@rpath/$(basename -- "$library")" "$library"
    run codesign --force --sign - "$library"
}

install_libtorch()
{
    local directory="$1"
    local archive openmp
    archive="$(fetch "$LIBTORCH_URL" "$LIBTORCH_SHA256" "libtorch-macos-arm64-$LIBTORCH_VERSION.zip")"
    extract "$archive" "$directory"
    # libtorch_cpu loads the OpenMP runtime from its Homebrew path, which most Macs do not have:
    # load the copy that LibTorch ships instead.
    openmp="$(otool -L "$directory/lib/libtorch_cpu.dylib" | awk '$1 ~ /\/libomp\.dylib$/ { print $1 }')"
    [[ -n "$openmp" ]] || error "libtorch_cpu.dylib no longer loads libomp.dylib"
    if [[ "$openmp" != @rpath/* ]]; then
        run install_name_tool -change "$openmp" @rpath/libomp.dylib "$directory/lib/libtorch_cpu.dylib"
        run codesign --force --sign - "$directory/lib/libtorch_cpu.dylib"
    fi
    use_rpath_install_name "$directory/lib/libomp.dylib"
}

install_vosk()
{
    local directory="$1"
    local build_directory="$2"
    local archive
    archive="$(fetch "$VOSK_URL" "$VOSK_SHA256" "vosk-osx-$VOSK_VERSION.zip")"
    extract "$archive" "$build_directory/vosk"
    mkdir -p "$directory/include" "$directory/lib"
    cp "$build_directory/vosk/vosk_api.h" "$directory/include/"
    run lipo "$build_directory/vosk/libvosk.dylib" -thin arm64 -output "$directory/lib/libvosk.dylib"
    use_rpath_install_name "$directory/lib/libvosk.dylib"
}

build_opencv()
{
    local directory="$1"
    local build_directory="$2"
    shift 2
    local archive
    archive="$(fetch "$OPENCV_URL" "$OPENCV_SHA256" "opencv-$OPENCV_VERSION.tar.gz")"
    extract "$archive" "$build_directory/source"
    # Recent macOS SDKs define TARGET_OS_MAC, which the bundled libpng takes for Mac OS 9 (fixed in
    # libpng 1.6.40). The bundled zlib has the same issue, so the SDK zlib is used instead.
    patch_source "$build_directory/source/3rdparty/libpng/pngpriv.h" \
        's/ \|\| defined\(TARGET_OS_MAC\)//'
    # The modules shipped in 3rdparty/opencv, with the camera read through AVFoundation and the
    # other image codecs built from OpenCV's bundled sources.
    run cmake -S "$build_directory/source" -B "$build_directory/build" "$@" \
        -DCMAKE_INSTALL_NAME_DIR=@rpath \
        -DBUILD_LIST=core,imgproc,imgcodecs,videoio,dnn,objdetect,calib3d,features2d,flann \
        -DBUILD_SHARED_LIBS=ON \
        -DBUILD_TESTS=OFF \
        -DBUILD_PERF_TESTS=OFF \
        -DBUILD_EXAMPLES=OFF \
        -DBUILD_DOCS=OFF \
        -DBUILD_JAVA=OFF \
        -DBUILD_opencv_apps=OFF \
        -DBUILD_opencv_python3=OFF \
        -DBUILD_ZLIB=OFF \
        -DBUILD_PNG=ON \
        -DBUILD_JPEG=ON \
        -DBUILD_PROTOBUF=ON \
        -DWITH_AVFOUNDATION=ON \
        -DWITH_FFMPEG=OFF \
        -DWITH_GSTREAMER=OFF \
        -DWITH_OPENCL=OFF \
        -DWITH_OPENEXR=OFF \
        -DWITH_OPENJPEG=OFF \
        -DWITH_JASPER=OFF \
        -DWITH_TIFF=OFF \
        -DWITH_WEBP=OFF \
        -DWITH_AVIF=OFF \
        -DWITH_EIGEN=OFF \
        -DWITH_LAPACK=OFF \
        -DWITH_ITT=OFF \
        -DWITH_IPP=OFF \
        -DWITH_OBSENSOR=OFF \
        -DOPENCV_GENERATE_PKGCONFIG=OFF \
        -DOPENCV_PYTHON_SKIP_DETECTION=ON
    run cmake --build "$build_directory/build" --parallel
    run cmake --install "$build_directory/build" --prefix "$directory"
}

build_tflite()
{
    local directory="$1"
    local build_directory="$2"
    shift 2
    local archive library
    archive="$(fetch "$TFLITE_URL" "$TFLITE_SHA256" "tensorflow-$TFLITE_VERSION.tar.gz")"
    extract "$archive" "$build_directory/source"
    # TensorFlow 2.15 passes std::abs<T> as a function, which recent libc++ versions reject.
    patch_source "$build_directory/source/tensorflow/lite/kernels/elementwise.cc" \
        's/std::abs<(float|int32_t)>,/[](\1 value) { return std::abs(value); },/g'
    # Same configuration as 3rdparty/tensorflowlite: XNNPACK off, pthreadpool comes from LibTorch.
    run cmake -S "$build_directory/source/tensorflow/lite" -B "$build_directory/build" "$@" \
        -DTFLITE_ENABLE_XNNPACK=OFF \
        -DTFLITE_ENABLE_GPU=OFF
    run cmake --build "$build_directory/build" --parallel --target tensorflow-lite
    mkdir -p "$directory/include" "$directory/lib/deps"
    cp "$build_directory/build/libtensorflow-lite.a" "$directory/lib/"
    (
        cd "$build_directory/build/_deps"
        find . -path './*-build/*' -name '*.a' -print
    ) | while IFS= read -r library; do
        mkdir -p "$directory/lib/deps/$(dirname -- "$library")"
        cp "$build_directory/build/_deps/$library" "$directory/lib/deps/$library"
    done
    (
        cd "$build_directory/source"
        find tensorflow/lite -name '*.h' -print
    ) | while IFS= read -r header; do
        mkdir -p "$directory/include/$(dirname -- "$header")"
        cp "$build_directory/source/$header" "$directory/include/$header"
    done
    cp -R "$build_directory/build/flatbuffers/include/flatbuffers" "$directory/include/"
}

platform_dependency()
{
    local name="$1"
    local sha256="$2"
    local builder="$3"
    shift 3
    local key directory
    key="$(
        {
            printf '%s\n' "$name" "$sha256" "$@"
            "$CXX_COMPILER" --version | head -n 1
            declare -f "$builder" extract patch_source use_rpath_install_name
        } | sha256 | cut -c1-16
    )"
    directory="$DEPS_ROOT/$name-$key"
    if [[ ! -f "$directory/.complete" ]]; then
        printf 'Building %s into %s\n' "$name" "$directory" >&2
        rm -rf "$directory" "$directory.build" ||
            error "cannot remove $directory, check its ownership and permissions"
        "$builder" "$directory" "$directory.build" "$@"
        touch "$directory/.complete"
        rm -rf "$directory.build"
    fi
    printf '%s' "$directory"
}

link_platform_dependencies()
{
    local prefix="$1"
    shift
    [[ "$(uname -m)" == arm64 ]] || error "macOS builds need an Apple silicon (arm64) Mac"
    local -a dependencies=(
        "libtorch libtorch-$LIBTORCH_VERSION $LIBTORCH_SHA256 install_libtorch"
        "opencv opencv-$OPENCV_VERSION $OPENCV_SHA256 build_opencv"
        "tensorflowlite tensorflowlite-$TFLITE_VERSION $TFLITE_SHA256 build_tflite"
        "vosk vosk-$VOSK_VERSION $VOSK_SHA256 install_vosk"
    )
    local dependency link name sha256 builder directory
    for dependency in "${dependencies[@]}"; do
        read -r link name sha256 builder <<<"$dependency"
        directory="$(platform_dependency "$name" "$sha256" "$builder" "$@")"
        ln -s "../$(basename -- "$directory")" "$prefix/$link"
    done
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
    if [[ "$PLATFORM" == Darwin ]]; then
        # Keep Homebrew libraries out of the dependencies, so they only need the macOS SDK.
        options+=(
            "-DCMAKE_OSX_DEPLOYMENT_TARGET=$MACOS_DEPLOYMENT_TARGET"
            "-DCMAKE_IGNORE_PREFIX_PATH=/opt/homebrew;/usr/local"
        )
    fi
    mapfile -t -O "${#options[@]}" options < <(compiler_launcher_options)
    build_yaml_cpp "$build_directory" "$prefix" "${options[@]}"
    build_cpr "$build_directory" "$prefix" "${options[@]}"
    build_portaudio "$build_directory" "$prefix" "${options[@]}"
    if [[ "$PLATFORM" == Darwin ]]; then
        link_platform_dependencies "$prefix" "${options[@]}"
    fi
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
