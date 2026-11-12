#!/usr/bin/env bash
# Runs the CI workflow (.github/workflows/ci.yml) locally with act, on a copy of the working tree in
# ufeel-act/ (git-ignored).
#
# Why a copy: the workflow runs `docker run -v "$GITHUB_WORKSPACE:/mnt" ...`. act forwards that to the
# host Docker daemon, so those steps act on the real directory act runs in. The copy keeps the
# root-owned build outputs away from the working tree and starts from a checkout-like state: the
# working tree as it is now (uncommitted changes included), without ignored files such as build/.

set -euo pipefail

readonly ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
readonly COPY_DIR="$ROOT_DIR/ufeel-act"
readonly RUNNER_IMAGE="ufeel-act-runner:22.04"

error()
{
    printf 'error: %s\n' "$*" >&2
    exit 1
}

step()
{
    printf '\n==> %s\n' "$*"
}

clean_copy()
{
    [[ -f "$COPY_DIR/build.sh" ]] || return 0
    # The Docker steps leave root-owned build/ and package/ directories, so clean as root.
    docker run --rm -v "$COPY_DIR:/mnt" -w /mnt "$RUNNER_IMAGE" ./build.sh clean
}

for tool in act docker git tar; do
    command -v "$tool" >/dev/null 2>&1 || error "required command '$tool' is not installed"
done

step "Building the runner image $RUNNER_IMAGE"
# GitHub's ubuntu-22.04 runner ships CMake 3.31 in /usr/local/bin. act's catthehacker image has
# none, and apt only provides 3.22, below the project's minimum (3.24).
docker build --quiet --tag "$RUNNER_IMAGE" - >/dev/null <<'EOF'
FROM catthehacker/ubuntu:act-22.04
RUN curl -fsSL https://github.com/Kitware/CMake/releases/download/v3.31.6/cmake-3.31.6-linux-x86_64.tar.gz \
    | tar -xz --strip-components=1 -C /usr/local
EOF

step "Removing the previous ufeel-act/"
clean_copy
rm -rf "$COPY_DIR" || error "cannot remove $COPY_DIR, check its ownership"

step "Copying the working tree to ufeel-act/"
mkdir "$COPY_DIR"
(
    cd "$ROOT_DIR"
    { git ls-files -z --recurse-submodules; git ls-files -z --others --exclude-standard; } |
        tar --null --files-from=- --ignore-failed-read -cf - | tar -xf - -C "$COPY_DIR"
)

# act reads the commit, branch, and repository name from git.
branch="$(git -C "$ROOT_DIR" rev-parse --abbrev-ref HEAD)"
[[ "$branch" != HEAD ]] || branch="ci-act-test"
git -C "$COPY_DIR" init --quiet --initial-branch="$branch"
if origin="$(git -C "$ROOT_DIR" remote get-url origin 2>/dev/null)"; then
    git -C "$COPY_DIR" remote add origin "$origin"
fi
git -C "$COPY_DIR" -c user.name=act -c user.email=act@localhost \
    commit --quiet --allow-empty --message "Local CI test"

event_file="$(mktemp)"
trap 'rm -f "$event_file"' EXIT
printf '{ "pull_request": { "base": { "ref": "main" }, "head": { "ref": "%s" } } }\n' "$branch" \
    >"$event_file"

step "Running the CI with act (pull_request to main)"
status=0
(
    cd "$COPY_DIR"
    act pull_request \
        --workflows .github/workflows/ci.yml \
        --job build-and-analysis \
        --eventpath "$event_file" \
        --bind \
        --platform "ubuntu-22.04=$RUNNER_IMAGE" \
        --pull=false
) || status=$?

step "Cleaning ufeel-act/ (./build.sh clean)"
clean_copy

if ((status == 0)); then
    step "CI passed"
else
    step "CI failed (act exit code $status)"
fi
exit "$status"
