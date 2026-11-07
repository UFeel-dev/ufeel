#!/usr/bin/env bash

# TODO: improve the overall script to reduce duplication and make sure messages are clear (e.g. give build command when package is ran without build)
# ! we also have several parts that are basically useless, and might have some parts that removes build data unnecessarily

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
PACKAGE_DIR="${ROOT_DIR}/package"
PACKAGE_PLATFORM="Linux-x86_64"
PACKAGE_PATH="${PACKAGE_DIR}/${PACKAGE_PLATFORM}"
CSHARP_PROJECT="${ROOT_DIR}/tests/integration"

CMAKE_GENERATOR="Ninja"
CMAKE_ARGS=("${@:2}")

COVERAGE_BUILD_DIR="${ROOT_DIR}/build-coverage"
MATRIX_BUILD_ROOT="${ROOT_DIR}/build-test-matrix"
MATRIX_PACKAGE_ROOT="${ROOT_DIR}/package-test-matrix"

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
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
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

run_integration_test()
{
    require_package # TODO: auto-package if missing

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

    find "${ROOT_DIR}/Library" "${ROOT_DIR}/Wrapper" "${ROOT_DIR}/tests" \
        -type f \
        \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" \) \
        -print0 |
        xargs -0 clang-format -i
}

clang_tidy()
{
    local fix_args=()
    local targets=()
    local include_tests=false
    local lint_build_dir="${BUILD_DIR}"
    local header_filter='.*(Library|Wrapper)/.*'

    while [ "$#" -gt 0 ]; do
        case "$1" in
            --fix)
                fix_args=(-fix)
                ;;
            --tests)
                include_tests=true
                ;;
            --)
                shift
                targets+=("$@")
                break
                ;;
            *)
                targets+=("$1")
                ;;
        esac
        shift
    done

    if [ "${include_tests}" = true ]; then
        configure_unit_tests
        lint_build_dir="${COVERAGE_BUILD_DIR}"
        header_filter='.*(Library|Wrapper|tests/unit)/.*'
    else
        require_build
    fi

    if [ "${#targets[@]}" -eq 0 ]; then
        targets=(
            'Library/.*'
            'Wrapper/.*'
        )
        if [ "${include_tests}" = true ]; then
            targets+=('tests/unit/.*')
        fi
    fi

    run-clang-tidy \
        -p "${lint_build_dir}" \
        "${fix_args[@]}" \
        -header-filter="${header_filter}" \
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
    local include_tests=false
    local targets=()

    while [ "$#" -gt 0 ]; do
        case "$1" in
            --tests)
                include_tests=true
                ;;
            --)
                shift
                targets+=("$@")
                break
                ;;
            *)
                targets+=("$1")
                ;;
        esac
        shift
    done

    local include_build_dir="${BUILD_DIR}"
    if [ "${include_tests}" = true ]; then
        configure_unit_tests
        include_build_dir="${COVERAGE_BUILD_DIR}"
    else
        require_build
    fi

    if [ "${#targets[@]}" -eq 0 ]; then
        targets=("${ROOT_DIR}/Library" "${ROOT_DIR}/Wrapper")
        if [ "${include_tests}" = true ]; then
            targets+=("${ROOT_DIR}/tests/unit")
        fi
    fi

    iwyu_tool.py \
        -p "${include_build_dir}" \
        "${targets[@]}" |
        fix_includes.py --nocomments --nosafe_headers
}

all()
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
    echo "  ./build.sh package"
    echo "  ./build.sh test"
    echo "  ./build.sh all"
    echo "  ./build.sh format [files...]"
    echo "  ./build.sh lint [--tests] [files...]"
    echo "  ./build.sh fix [--tests] [files...]"
    echo "  ./build.sh include-fix [--tests] [files...]"
    echo "  ./build.sh run-unit-tests"
    echo "  ./build.sh run-integration-tests"
    echo "  ./build.sh run-feature-matrix"
}

configure_unit_tests()
{
    echo "Configuring coverage build..."
    cmake \
        -S "${ROOT_DIR}" \
        -B "${COVERAGE_BUILD_DIR}" \
        -G "${CMAKE_GENERATOR}" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_CXX_COMPILER=clang++-22 \
        -DUFEEL_BUILD_TESTS=ON \
        -DUFEEL_ENABLE_COVERAGE=ON \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5
}

run_unit_tests()
{
    local runtime_library_path="${ROOT_DIR}/3rdparty/opencv/lib:${ROOT_DIR}/3rdparty/libtorch/lib:${ROOT_DIR}/3rdparty/portaudio/lib:${COVERAGE_BUILD_DIR}/yaml-cpp"
    if [ -n "${LD_LIBRARY_PATH:-}" ]; then
        runtime_library_path="${runtime_library_path}:${LD_LIBRARY_PATH}"
    fi

    configure_unit_tests

    echo "Building unit tests..."
    LD_LIBRARY_PATH="${runtime_library_path}" \
        cmake --build "${COVERAGE_BUILD_DIR}" --parallel

    echo "Running unit tests..."
    rm -f "${COVERAGE_BUILD_DIR}"/coverage-*.profraw

    LD_LIBRARY_PATH="${runtime_library_path}" \
    LLVM_PROFILE_FILE="${COVERAGE_BUILD_DIR}/coverage-%p.profraw" \
        ctest \
            --test-dir "${COVERAGE_BUILD_DIR}" \
            --output-on-failure

    echo "Merging coverage profiles..."
    local llvm_profdata
    local llvm_cov
    llvm_profdata="$(command -v llvm-profdata-22 || command -v llvm-profdata)"
    llvm_cov="$(command -v llvm-cov-22 || command -v llvm-cov)"

    "${llvm_profdata}" merge -sparse \
        "${COVERAGE_BUILD_DIR}"/coverage-*.profraw \
        -o "${COVERAGE_BUILD_DIR}/coverage.profdata"

    echo "Generating coverage report..."
    "${llvm_cov}" report \
        "${COVERAGE_BUILD_DIR}/tests/unit/Library/run_library_unit_tests" \
        -object="${COVERAGE_BUILD_DIR}/tests/unit/Wrapper/run_wrapper_unit_tests" \
        -object="${COVERAGE_BUILD_DIR}/libufeel_detection_library.a" \
        -object="${COVERAGE_BUILD_DIR}/Wrapper/libufeel_wrapper.so" \
        -instr-profile="${COVERAGE_BUILD_DIR}/coverage.profdata" \
        -ignore-filename-regex='(/tests/|googletest|/usr/|3rdparty/|_deps/)'
}

run_matrix_configuration()
{
    local name="$1"
    local feature_test="$2"
    shift 2

    local matrix_build_dir="${MATRIX_BUILD_ROOT}/${name}"
    local matrix_package_dir="${MATRIX_PACKAGE_ROOT}/${name}"
    local runtime_library_path="${ROOT_DIR}/3rdparty/opencv/lib:${ROOT_DIR}/3rdparty/libtorch/lib:${ROOT_DIR}/3rdparty/portaudio/lib:${matrix_build_dir}/yaml-cpp"
    local package_library_path="${matrix_package_dir}/dependencies"

    echo
    echo "=== Testing ${name} ==="
    rm -rf "${matrix_build_dir}" "${matrix_package_dir}"

    cmake \
        -S "${ROOT_DIR}" \
        -B "${matrix_build_dir}" \
        -G "${CMAKE_GENERATOR}" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_COMPILER=clang++-22 \
        -DUFEEL_BUILD_TESTS=ON \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
        "$@"

    LD_LIBRARY_PATH="${runtime_library_path}" \
        cmake --build "${matrix_build_dir}" --parallel

    cmake \
        --install "${matrix_build_dir}" \
        --prefix "${matrix_package_dir}"

    LD_LIBRARY_PATH="${runtime_library_path}" \
        ctest \
            --test-dir "${matrix_build_dir}" \
            --output-on-failure

    UFEEL_PACKAGE_PATH="${matrix_package_dir}/libufeel_wrapper.so" \
    LD_LIBRARY_PATH="${package_library_path}:${runtime_library_path}" \
        dotnet run \
            --project "${CSHARP_PROJECT}" \
            -p:OutputPath="${matrix_build_dir}/dotnet-output/" \
            -p:IntermediateOutputPath="${matrix_build_dir}/dotnet-intermediate/" \
            -- --feature-test "${feature_test}"
}

run_feature_matrix()
{
    rm -rf "${MATRIX_BUILD_ROOT}" "${MATRIX_PACKAGE_ROOT}"

    run_matrix_configuration \
        all \
        all \
        -DUFEEL_ENABLE_EMOTIONS=ON \
        -DUFEEL_ENABLE_EYE_TRACKING=ON \
        -DUFEEL_ENABLE_SPEECH_TO_TEXT=ON \
        -DUFEEL_ENABLE_HEART_RATE=ON

    run_matrix_configuration \
        emotions \
        emotions \
        -DUFEEL_ENABLE_EMOTIONS=ON \
        -DUFEEL_ENABLE_EYE_TRACKING=OFF \
        -DUFEEL_ENABLE_SPEECH_TO_TEXT=OFF \
        -DUFEEL_ENABLE_HEART_RATE=OFF

    run_matrix_configuration \
        eye-tracking \
        eye-tracking \
        -DUFEEL_ENABLE_EMOTIONS=OFF \
        -DUFEEL_ENABLE_EYE_TRACKING=ON \
        -DUFEEL_ENABLE_SPEECH_TO_TEXT=OFF \
        -DUFEEL_ENABLE_HEART_RATE=OFF

    run_matrix_configuration \
        speech-to-text \
        speech-to-text \
        -DUFEEL_ENABLE_EMOTIONS=OFF \
        -DUFEEL_ENABLE_EYE_TRACKING=OFF \
        -DUFEEL_ENABLE_SPEECH_TO_TEXT=ON \
        -DUFEEL_ENABLE_HEART_RATE=OFF

    run_matrix_configuration \
        heart-rate \
        heart-rate \
        -DUFEEL_ENABLE_EMOTIONS=OFF \
        -DUFEEL_ENABLE_EYE_TRACKING=OFF \
        -DUFEEL_ENABLE_SPEECH_TO_TEXT=OFF \
        -DUFEEL_ENABLE_HEART_RATE=ON
}

test()
{
    run_unit_tests
    run_integration_test
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
    run-integration-tests)
        run_integration_test
        ;;
    run-unit-tests)
        run_unit_tests
        ;;
    run-feature-matrix)
        run_feature_matrix
        ;;
    *)
        usage
        exit 1
        ;;
esac
