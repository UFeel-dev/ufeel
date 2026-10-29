#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
PACKAGE_DIR="${ROOT_DIR}/package"
PACKAGE_PLATFORM="Linux-x86_64"
PACKAGE_PATH="${PACKAGE_DIR}/${PACKAGE_PLATFORM}"
CSHARP_PROJECT="${ROOT_DIR}/csharp"

CMAKE_GENERATOR="Ninja"
CMAKE_ARGS=("${@:2}")

COVERAGE_BUILD_DIR="${ROOT_DIR}/build-coverage"
LLVM_BIN="/usr/lib/llvm-22/bin"

require_build()
{
    if [ ! -f "${BUILD_DIR}/build.ninja" ]; then
        echo "Build directory is not configured."
        echo "Run: ./build.sh configure"
        exit 1
    fi
}

require_package()
{
    if [ ! -f "${PACKAGE_PATH}/libufeel_wrapper.so" ]; then
        echo "Package does not exist."
        echo "Run: ./build.sh package"
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
        -DCMAKE_CXX_COMPILER=clang++-22 \
        -DUFEEL_BUILD_TESTS=OFF \
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

package()
{
    require_build

    echo "Cleaning package..."
    rm -rf "${PACKAGE_DIR}"

    echo "Installing package..."
    cmake \
        --install "${BUILD_DIR}" \
        --prefix "${PACKAGE_PATH}"
}

test()
{
    require_package

    echo "Running C# integration test..."
    UFEEL_PACKAGE_PATH="${PACKAGE_PATH}/libufeel_wrapper.so" \
    LD_LIBRARY_PATH="" \
    dotnet run \
        --project "${CSHARP_PROJECT}"
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
    package
    test # Maybe remove test from all and update documentation
}

usage()
{
    echo "Usage:"
    echo "  ./build.sh configure [cmake-options]"
    echo "  ./build.sh build"
    echo "  ./build.sh package"
    echo "  ./build.sh test"
    echo "  ./build.sh all"
    echo "  ./build.sh format [files...]"
    echo "  ./build.sh lint [files...]"
    echo "  ./build.sh fix [files...]"
    echo "  ./build.sh include-fix [files...]"
    echo "  ./build.sh run-unit-tests"
}

run_unit_tests()
{
    echo "Configuring coverage build..."
    cmake \
        -S "${ROOT_DIR}" \
        -B "${COVERAGE_BUILD_DIR}" \
        -G "${CMAKE_GENERATOR}" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_CXX_COMPILER=clang++-22 \
        -DUFEEL_BUILD_TESTS=ON \
        -DUFEEL_ENABLE_COVERAGE=ON

    echo "Building unit tests..."
    cmake --build "${COVERAGE_BUILD_DIR}" --parallel

    echo "Running unit tests..."
    rm -f "${COVERAGE_BUILD_DIR}"/coverage-*.profraw

    LLVM_PROFILE_FILE="${COVERAGE_BUILD_DIR}/coverage-%p.profraw" \
        ctest \
            --test-dir "${COVERAGE_BUILD_DIR}" \
            --output-on-failure

    echo "Merging coverage profiles..."
    "${LLVM_BIN}/llvm-profdata" merge -sparse \
        "${COVERAGE_BUILD_DIR}"/coverage-*.profraw \
        -o "${COVERAGE_BUILD_DIR}/coverage.profdata"

    echo "Generating coverage report..."
    "${LLVM_BIN}/llvm-cov" report \
        "${COVERAGE_BUILD_DIR}/tests/unit/run_unit_tests" \
        -object="${COVERAGE_BUILD_DIR}/libufeel_detection_library.a" \
        -instr-profile="${COVERAGE_BUILD_DIR}/coverage.profdata" \
        -ignore-filename-regex='(/tests/|googletest|/usr/|3rdparty/)'
}

case "${1:-}" in
    configure)
        configure
        ;;
    build)
        build
        ;;
    package)
        package
        ;;
    test)
        test
        ;;
    all)
        all
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
    run-unit-tests)
        run_unit_tests
        ;;
    *)
        usage
        exit 1
        ;;
esac
