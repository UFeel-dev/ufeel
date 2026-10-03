#!/usr/bin/env bash

set -e

BUILD_DIR="build"
CMAKE_ARGS=("${@:2}")
CSHARP_PROJECT="csharp"

require_build()
{
    if [ ! -f "$BUILD_DIR/compile_commands.json" ]; then
        echo "Build directory is not configured."
        echo "Run: ./build.sh build"
        exit 1
    fi
}

clang_tidy()
{
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
        -p "$BUILD_DIR" \
        "${fix_args[@]}" \
        -header-filter='.*(Library|Wrapper)/.*' \
        -exclude-header-filter='.*3rdparty/.*' \
        "${targets[@]}"
}

build()
{
    echo "Cleaning build..."
    rm -rf "$BUILD_DIR"

    echo "Configuring CMake..."
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    cmake .. "${CMAKE_ARGS[@]}"
    cd ..
}

compile()
{
    echo "Compiling..."
    cd "$BUILD_DIR"
    make -j8
    cd ..
}

run()
{
    echo "Running C# test..."
    dotnet run --project "$CSHARP_PROJECT"
}

format()
{
    if [ "$#" -gt 0 ]; then
        clang-format -i "$@"
        return
    fi

    find Library Wrapper \
        -type f \
        \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" \) \
        -print0 |
        xargs -0 clang-format -i
}

lint()
{
    require_build
    clang_tidy "$@"
}

fix()
{
    require_build
    clang_tidy --fix "$@"
}

include_fix()
{
    require_build

    if [ "$#" -eq 0 ]; then
        set -- Library Wrapper
    fi

    iwyu_tool.py \
        -p "$BUILD_DIR" \
        "$@" |
        fix_includes.py --nocomments --nosafe_headers
}

case "$1" in

build)
    build
    ;;

compile)
    compile
    ;;

run)
    run
    ;;

all)
    build
    compile
    run
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

include_fix)
    include_fix "${@:2}"
    ;;

*)
    echo "Usage:"
    echo "  ./build.sh build"
    echo "  ./build.sh compile"
    echo "  ./build.sh run"
    echo "  ./build.sh all"
    echo "  ./build.sh format [files...]"
    echo "  ./build.sh lint [files...]"
    echo "  ./build.sh fix [files...]"
    echo "  ./build.sh include_fix [files...]"
    exit 1
    ;;

esac
