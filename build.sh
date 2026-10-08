#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
PACKAGE_DIR="${ROOT_DIR}/package"
CSHARP_PROJECT="${ROOT_DIR}/csharp"

CMAKE_GENERATOR="Ninja"
CMAKE_ARGS=("${@:2}")

require_build()
{
    if [ ! -f "${BUILD_DIR}/build.ninja" ]; then
        echo "Build directory is not configured."
        echo "Run: ./build.sh configure"
        exit 1
    fi
}

configure()
{
    echo "Cleaning build..."
    rm -rf "${BUILD_DIR}"

    echo "Configuring CMake..."
    cmake \
        -S "${ROOT_DIR}" \
        -B "${BUILD_DIR}" \
        -G "${CMAKE_GENERATOR}" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_C_COMPILER=clang-22 \
        -DCMAKE_CXX_COMPILER=clang++-22 \
        "${CMAKE_ARGS[@]}"
}

build()
{
    require_build

    echo "Building..."
    cmake \
        --build "${BUILD_DIR}" \
        --parallel
}

test()
{
    require_build

    echo "Running C# test..."
    dotnet run --project "${CSHARP_PROJECT}"
}

package()
{
    require_build

    echo "Cleaning package..."
    rm -rf "${PACKAGE_DIR}"

    echo "Installing package..."
    cmake \
        --install "${BUILD_DIR}" \
        --prefix "${PACKAGE_DIR}/Linux-x86_64"
}

format()
{
    if [ "$#" -gt 0 ]; then
        clang-format -i "$@"
        return
    fi

    find "${ROOT_DIR}/Library" "${ROOT_DIR}/Wrapper" \
        -type f \
        \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" \) \
        -print0 |
        xargs -0 clang-format -i
}

clang_tidy()
{
    require_build

    local fix_args=()
    local targets=("$@")

    if [ "${1:-}" = "--fix" ]; then
        fix_args=(-fix)
        shift
        targets=("$@")
    fi

    if [ "${#targets[@]}" -eq 0 ]; then
        targets=(
            'Library/.*'
            'Wrapper/.*'
        )
    fi

    run-clang-tidy \
        -p "${BUILD_DIR}" \
        "${fix_args[@]}" \
        -header-filter='.*(Library|Wrapper)/.*' \
        -exclude-header-filter='.*3rdparty/.*' \
        "${targets[@]}"
}

lint()
{
    clang_tidy "$@"
}

fix()
{
    clang_tidy --fix "$@"
}

include_fix()
{
    require_build

    if [ "$#" -eq 0 ]; then
        set -- "${ROOT_DIR}/Library" "${ROOT_DIR}/Wrapper"
    fi

    iwyu_tool.py \
        -p "${BUILD_DIR}" \
        "$@" |
        fix_includes.py --nocomments --nosafe_headers
}

all()
{
    configure
    build
    test
}

package_all()
{
    configure
    build
    package
}

usage()
{
    echo "Usage:"
    echo "  ./build.sh configure [cmake-options]"
    echo "  ./build.sh build"
    echo "  ./build.sh test"
    echo "  ./build.sh all"
    echo "  ./build.sh package"
    echo "  ./build.sh package-all"
    echo "  ./build.sh format [files...]"
    echo "  ./build.sh lint [files...]"
    echo "  ./build.sh fix [files...]"
    echo "  ./build.sh include-fix [files...]"
}

case "${1:-}" in
    configure)
        configure
        ;;
    build)
        build
        ;;
    test)
        test
        ;;
    all)
        all
        ;;
    package)
        package
        ;;
    package-all)
        package_all
        ;;
    format)
        format "${@:2}"
        ;;
    lint)
        lint "${@:2}"
        ;;
    fix)
        fix "${@:2}"
        ;;
    include-fix)
        include_fix "${@:2}"
        ;;
    *)
        usage
        exit 1
        ;;
esac
