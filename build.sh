#!/usr/bin/env bash

set -euo pipefail

readonly ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
readonly BUILD_ROOT="${ROOT_DIR}/build"
readonly PACKAGE_ROOT="${ROOT_DIR}/package"
readonly CSHARP_INTEGRATION_PROJECT="${ROOT_DIR}/tests/integration/csharp/UFeel.FeatureTests"
readonly TEST_HARNESS_PROJECT="${ROOT_DIR}/tests/test-harnesses/csharp/UFeel.TestHarness"
readonly GENERATOR="Ninja"
readonly CXX_COMPILER="clang++-22"
readonly FEATURES=(emotions eye-tracking speech-to-text heart-rate)

configuration="Release"
clean=false
command=""
declare -a cmake_extra=()
declare -A feature_state=(
    [emotions]=ON
    [eye-tracking]=ON
    [speech-to-text]=ON
    [heart-rate]=ON
)
feature_selection=false

feature_option_name()
{
    printf '%s' "$1" | tr '[:lower:]-' '[:upper:]_'
}

usage()
{
    cat <<'EOF'
Usage: ./build.sh [options] <command> [-- <CMake options>]

Commands:
  all                     Configure, build, and package
  configure               Configure a build directory
  build                   Build a configured directory
  package                 Install a built package
  test                    Run all tests that need no camera, microphone or fixed port
  test-unit               Run C++ unit tests
  test-integration        Run C++ and C# integration tests
  test-e2e                Run C++ end-to-end tests (hardware: camera, microphone, ports)
  test-harness            Build the interactive C# test harness
  test-feature-matrix     Test all feature configurations through C# integration tests
  coverage                Run all native tests and generate coverage
  format                  Format C++ files
  lint                    Run clang-tidy
  fix                     Run clang-tidy with fixes
  include-fix             Run include-what-you-use fixes
  clean                   Remove build and package directories

Options:
  --config CONFIG         Debug or Release (default: Release)
  --clean                 Remove the selected build/package state first
  --feature FEATURE       Build only this feature (repeatable)
  --without FEATURE       Disable this feature (repeatable)
  --all-features          Enable every feature
  -h, --help              Show this help

Features:
  emotions, eye-tracking, speech-to-text, heart-rate
EOF
}

error()
{
    printf 'error: %s\n' "$*" >&2
    exit 1
}

hint()
{
    printf 'hint: %s\n' "$*" >&2
}

require_command()
{
    command -v "$1" >/dev/null 2>&1 ||
        error "required command '$1' is not installed"
}

require_commands()
{
    local tool
    for tool in "$@"; do
        require_command "$tool"
    done
}

remove_generated()
{
    local directory
    for directory in "$@"; do
        [[ -e "$directory" ]] || continue
        rm -rf "$directory" 2>/dev/null ||
            error "cannot remove $directory, check its ownership and permissions"
    done
}

valid_feature()
{
    local feature="$1"
    local candidate
    for candidate in "${FEATURES[@]}"; do
        [[ "$candidate" == "$feature" ]] && return 0
    done
    return 1
}

set_feature()
{
    local feature="$1"
    valid_feature "$feature" ||
        error "unknown feature '$feature' (choose: ${FEATURES[*]})"
    feature_state["$feature"]="$2"
}

parse_args()
{
    while (($#)); do
        case "$1" in
            -h|--help)
                usage
                exit 0
                ;;
            --config)
                (($# >= 2)) || error "--config requires Debug or Release"
                configuration="$2"
                shift
                ;;
            --config=*)
                configuration="${1#*=}"
                ;;
            --clean)
                clean=true
                ;;
            --all-features)
                for feature in "${FEATURES[@]}"; do
                    feature_state["$feature"]=ON
                done
                feature_selection=false
                ;;
            --feature)
                (($# >= 2)) || error "--feature requires a feature name"
                if [[ "$feature_selection" == false ]]; then
                    for feature in "${FEATURES[@]}"; do
                        feature_state["$feature"]=OFF
                    done
                    feature_selection=true
                fi
                set_feature "$2" ON
                shift
                ;;
            --feature=*)
                if [[ "$feature_selection" == false ]]; then
                    for feature in "${FEATURES[@]}"; do
                        feature_state["$feature"]=OFF
                    done
                    feature_selection=true
                fi
                set_feature "${1#*=}" ON
                ;;
            --without)
                (($# >= 2)) || error "--without requires a feature name"
                set_feature "$2" OFF
                shift
                ;;
            --without=*)
                set_feature "${1#*=}" OFF
                ;;
            --)
                shift
                cmake_extra+=("$@")
                break
                ;;
            configure|build|package|all|test|clean|format|lint|fix|include-fix|\
            test-unit|test-integration|test-e2e|test-harness|\
            test-feature-matrix|coverage)
                [[ -z "$command" ]] || error "only one command may be specified"
                command="$1"
                ;;
            *)
                if [[ -n "$command" ]]; then
                    cmake_extra+=("$1")
                else
                    error "unknown command or option '$1'"
                fi
                ;;
        esac
        shift
    done
    [[ -n "$command" ]] || {
        usage
        exit 0
    }
    [[ "$configuration" == Debug || "$configuration" == Release ]] ||
        error "configuration must be Debug or Release"
}

build_dir()
{
    printf '%s/%s' "$BUILD_ROOT" "${1,,}"
}

package_dir()
{
    printf '%s/%s' "$PACKAGE_ROOT" "${1,,}"
}

require_configured()
{
    local directory="$1"
    [[ -f "$directory/build.ninja" ]] ||
        error "$directory is not configured, run './build.sh configure --config ${2:-$configuration}'"
}

require_built()
{
    local directory="$1"
    require_configured "$directory"
    [[ -f "$directory/Wrapper/libufeel_wrapper.so" ]] ||
        error "$directory is not built, run './build.sh build --config ${2:-$configuration}'"
}

require_package()
{
    local directory="$1"
    [[ -f "$directory/libufeel_wrapper.so" ]] ||
        error "$directory is not packaged, run './build.sh all --config ${2:-$configuration}'"
}

feature_options()
{
    local feature
    for feature in "${FEATURES[@]}"; do
        printf '%s\n' "-DUFEEL_ENABLE_$(feature_option_name "$feature")=${feature_state[$feature]}"
    done
}

runtime_path()
{
    local build_directory="$1"
    local package_directory="${2:-}"
    local -a paths=()
    if [[ -n "$package_directory" ]]; then
        paths+=("$package_directory/dependencies")
    else
        paths+=(
            "$ROOT_DIR/3rdparty/opencv/lib"
            "$ROOT_DIR/3rdparty/libtorch/lib"
            "$ROOT_DIR/3rdparty/portaudio/lib"
            "$build_directory/yaml-cpp"
        )
    fi
    local path
    local -a existing=()
    for path in "${paths[@]}"; do
        [[ -d "$path" ]] && existing+=("$path")
    done
    [[ -n "${LD_LIBRARY_PATH:-}" ]] && existing+=("$LD_LIBRARY_PATH")
    (IFS=:; printf '%s' "${existing[*]}")
}

run()
{
    printf '+'
    printf ' %q' "$@"
    printf '\n'
    "$@"
}

run_with_environment()
{
    local name="$1"
    local value="$2"
    shift 2
    printf '+ env %q=%q' "$name" "$value"
    printf ' %q' "$@"
    printf '\n'
    env "$name=$value" "$@"
}

configure()
{
    local directory="${1:-$(build_dir "$configuration")}"
    "$clean" && remove_generated "$directory"
    local -a options=()
    mapfile -t options < <(feature_options)
    run cmake -S "$ROOT_DIR" -B "$directory" -G "$GENERATOR" \
        "-DCMAKE_BUILD_TYPE=$configuration" \
        "-DCMAKE_CXX_COMPILER=$CXX_COMPILER" \
        -DUFEEL_BUILD_TESTS=OFF \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
        "${options[@]}" "${cmake_extra[@]}"
}

build()
{
    local directory="${1:-$(build_dir "$configuration")}"
    require_configured "$directory"
    run cmake --build "$directory" --parallel
}

package()
{
    local directory="${1:-$(build_dir "$configuration")}"
    local destination="${2:-$(package_dir "$configuration")}"
    require_built "$directory"
    "$clean" && remove_generated "$destination"
    run cmake --install "$directory" --prefix "$destination"
}

test_csharp_integration()
{
    local package_directory
    local build_directory

    package_directory="$(package_dir "$configuration")"
    build_directory="$(build_dir "$configuration")"

    require_commands dotnet

    if [[ ! -f "$package_directory/libufeel_wrapper.so" ]]; then
        configure
        build
        package
    fi

    run dotnet build "$CSHARP_INTEGRATION_PROJECT" \
        --configuration "$configuration"

    run_with_environment UFEEL_PACKAGE_PATH "$package_directory/libufeel_wrapper.so" \
        env LD_LIBRARY_PATH="$(runtime_path "$build_directory" "$package_directory")" \
        dotnet run \
            --project "$CSHARP_INTEGRATION_PROJECT" \
            --configuration "$configuration" \
            --no-build -- --feature-test all
}

test_harness()
{
    local package_directory
    local build_directory

    package_directory="$(package_dir "$configuration")"
    build_directory="$(build_dir "$configuration")"

    require_commands dotnet

    if [[ ! -f "$package_directory/libufeel_wrapper.so" ]]; then
        configure
        build
        package
    fi

    run dotnet build "$TEST_HARNESS_PROJECT" \
        --configuration "$configuration"

    run_with_environment UFEEL_PACKAGE_PATH "$package_directory/libufeel_wrapper.so" \
        env LD_LIBRARY_PATH="$(runtime_path "$build_directory" "$package_directory")" \
        dotnet run \
            --project "$TEST_HARNESS_PROJECT" \
            --configuration "$configuration" \
            --no-build
}

configure_test_build()
{
    local directory="${1:-$BUILD_ROOT/tests}"
    "$clean" && remove_generated "$directory"
    local -a options=()
    mapfile -t options < <(feature_options)
    run cmake -S "$ROOT_DIR" -B "$directory" -G "$GENERATOR" \
        "-DCMAKE_BUILD_TYPE=$configuration" \
        "-DCMAKE_CXX_COMPILER=$CXX_COMPILER" \
        -DUFEEL_BUILD_TESTS=ON \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
        "${options[@]}"
}

run_native_tests()
{
    local directory="$BUILD_ROOT/tests"
    [[ -f "$directory/build.ninja" ]] || configure_test_build "$directory"
    build "$directory"
    run_with_environment LD_LIBRARY_PATH "$(runtime_path "$directory")" \
        ctest --test-dir "$directory" --output-on-failure "$@"
}

test_all()
{
    run_native_tests --label-exclude '^hardware$'
    test_csharp_integration
}

test_unit()
{
    run_native_tests --label-regex '^unit$'
}

test_integration()
{
    run_native_tests --label-regex '^integration$'
    test_csharp_integration
}

test_e2e()
{
    run_native_tests --label-regex '^e2e$'
}

run_native_tests_with_coverage()
{
    require_commands cmake ctest
    local directory="$BUILD_ROOT/coverage"
    local old_configuration="$configuration"
    configuration=Debug
    run cmake -S "$ROOT_DIR" -B "$directory" -G "$GENERATOR" \
        -DCMAKE_BUILD_TYPE=Debug "-DCMAKE_CXX_COMPILER=$CXX_COMPILER" \
        -DUFEEL_BUILD_TESTS=ON -DUFEEL_ENABLE_COVERAGE=ON \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5
    build "$directory"
    rm -f "$directory"/coverage-*.profraw
    run_with_environment LLVM_PROFILE_FILE "$directory/coverage-%p.profraw" \
        env LD_LIBRARY_PATH="$(runtime_path "$directory")" \
        ctest --test-dir "$directory" --output-on-failure
    local profdata
    local cov
    profdata="$(command -v llvm-profdata-22 || command -v llvm-profdata)" ||
        error "llvm-profdata is required for coverage"
    cov="$(command -v llvm-cov-22 || command -v llvm-cov)" ||
        error "llvm-cov is required for coverage"
    local -a profiles=("$directory"/coverage-*.profraw)
    run "$profdata" merge -sparse "${profiles[@]}" -o "$directory/coverage.profdata"
    run "$cov" report "$directory/tests/unit/cpp/Library/run_library_unit_tests" \
        -object="$directory/tests/unit/cpp/Wrapper/run_wrapper_unit_tests" \
        -object="$directory/tests/integration/cpp/run_library_integration_tests" \
        -object="$directory/tests/integration/cpp/run_wrapper_integration_tests" \
        -object="$directory/tests/e2e/cpp/run_wrapper_e2e_tests" \
        -object="$directory/libufeel_detection_library.a" \
        -object="$directory/Wrapper/libufeel_wrapper.so" \
        -instr-profile="$directory/coverage.profdata" \
        -ignore-filename-regex='(/tests/|googletest|/usr/|3rdparty/|_deps/)'
    configuration="$old_configuration"
}

test_feature_matrix()
{
    local matrix="$BUILD_ROOT/matrix"
    local packages="$PACKAGE_ROOT/matrix"
    remove_generated "$matrix" "$packages"
    local name feature
    for name in all "${FEATURES[@]}"; do
        local directory="$matrix/$name"
        local destination="$packages/$name"
        local -a options=()
        for feature in "${FEATURES[@]}"; do
            [[ "$name" == all || "$name" == "$feature" ]] &&
                options+=("-DUFEEL_ENABLE_$(feature_option_name "$feature")=ON") ||
                options+=("-DUFEEL_ENABLE_$(feature_option_name "$feature")=OFF")
        done
        run cmake -S "$ROOT_DIR" -B "$directory" -G "$GENERATOR" \
            -DCMAKE_BUILD_TYPE=Release "-DCMAKE_CXX_COMPILER=$CXX_COMPILER" \
            -DUFEEL_BUILD_TESTS=ON -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
            "${options[@]}"
        run cmake --build "$directory" --parallel
        run cmake --install "$directory" --prefix "$destination"
        run_with_environment LD_LIBRARY_PATH "$(runtime_path "$directory" "$destination")" \
            ctest --test-dir "$directory" --label-exclude '^hardware$' --output-on-failure
        run_with_environment UFEEL_PACKAGE_PATH "$destination/libufeel_wrapper.so" \
            env LD_LIBRARY_PATH="$(runtime_path "$directory" "$destination")" \
            dotnet run --project "$CSHARP_INTEGRATION_PROJECT" \
                -p:OutputPath="$directory/dotnet-output/" \
                -p:IntermediateOutputPath="$directory/dotnet-intermediate/" \
                -- --feature-test "$name"
    done
}

format_sources()
{
    require_commands clang-format
    if (("$#")); then
        run clang-format -i "$@"
        return
    fi
    local -a sources=()
    while IFS= read -r -d '' file; do sources+=("$file"); done < <(
        find "$ROOT_DIR/Library" "$ROOT_DIR/Wrapper" "$ROOT_DIR/tests" -type f \
            \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) -print0
    )
    run clang-format -i "${sources[@]}"
}

lint()
{
    local include_tests=false
    local apply_fixes=false
    local -a targets=()
    local target

    for target in "$@"; do
        [[ "$target" == --tests ]] && include_tests=true || targets+=("$target")
        [[ "$target" == --fix ]] && apply_fixes=true
    done

    if "$apply_fixes"; then
        local filtered=()
        for target in "${targets[@]}"; do
            [[ "$target" != --fix ]] && filtered+=("$target")
        done
        targets=("${filtered[@]}")
    fi

    local directory
    if "$include_tests"; then
        directory="$BUILD_ROOT/coverage"
        [[ -f "$directory/build.ninja" ]] || run_native_tests_with_coverage
    else
        directory="$(build_dir Release)"
    fi

    require_configured "$directory"

    if ((${#targets[@]} == 0)); then
        targets=(Library/.* Wrapper/.*)
        "$include_tests" && targets+=( 'tests/(unit|integration|e2e)/.*' )
    fi

    local -a options=(
        run-clang-tidy
        -p "$directory"
        -header-filter='.*(Library|Wrapper|tests/(unit|integration|e2e))/.*'
    )

    "$apply_fixes" && options+=(-fix)
    run "${options[@]}" "${targets[@]}"
}

include_fix()
{
    local directory="$(build_dir Release)"
    require_configured "$directory"
    local -a targets=("$ROOT_DIR/Library" "$ROOT_DIR/Wrapper")
    (("$#" == 0)) || targets=("$@")
    printf '+'
    printf ' %q' iwyu_tool.py -p "$directory" "${targets[@]}"
    printf ' |'
    printf ' %q' fix_includes.py --nocomments --nosafe_headers
    printf '\n'
    iwyu_tool.py -p "$directory" "${targets[@]}" |
        fix_includes.py --nocomments --nosafe_headers
}

dispatch()
{
    case "$command" in
        configure) configure ;;
        build) build ;;
        package) package ;;
        all) configure; build; package ;;
        test) test_all ;;
        clean) remove_generated "$BUILD_ROOT" "$PACKAGE_ROOT" ;;
        format) format_sources "${cmake_extra[@]}" ;;
        lint) lint "${cmake_extra[@]}" ;;
        fix) lint --fix "${cmake_extra[@]}" ;;
        include-fix) include_fix "${cmake_extra[@]}" ;;
        test-unit) test_unit ;;
        test-integration) test_integration ;;
        test-e2e) test_e2e ;;
        test-harness) test_harness ;;
        test-feature-matrix) test_feature_matrix ;;
        coverage) run_native_tests_with_coverage ;;
        *) error "unsupported command '$command'" ;;
    esac
}

parse_args "$@"
case "$command" in
    clean) ;;
    format) require_commands clang-format ;;
    lint|fix) require_commands cmake run-clang-tidy ;;
    include-fix) require_commands cmake iwyu_tool.py fix_includes.py ;;
    test-unit|test-integration|test-e2e|coverage) require_commands cmake ctest ;;
    test) require_commands cmake ctest dotnet ;;
    test-harness) require_commands cmake dotnet ;;
    test-feature-matrix) require_commands cmake ctest dotnet ;;
    *) require_commands cmake ;;
esac
dispatch
