#!/usr/bin/env bash

set -euo pipefail

readonly ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
readonly COPY_DIR="$ROOT_DIR/ufeel-act"
readonly RUNNER_IMAGE="catthehacker/ubuntu:act-22.04"
readonly JOBS=(lint build)

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
    docker run --rm -v "$COPY_DIR:/mnt" -w /mnt "$RUNNER_IMAGE" ./build.sh clean
}

for tool in act docker git tar; do
    command -v "$tool" >/dev/null 2>&1 || error "required command '$tool' is not installed"
done

if ! docker image inspect "$RUNNER_IMAGE" >/dev/null 2>&1; then
    step "Pulling the runner image $RUNNER_IMAGE"
    docker pull "$RUNNER_IMAGE"
fi

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

status=0
for job in "${JOBS[@]}"; do
    step "Running the CI job '$job' with act (pull_request to main)"
    job_status=0
    (
        cd "$COPY_DIR"
        act pull_request \
            --workflows .github/workflows/ci.yml \
            --job "$job" \
            --eventpath "$event_file" \
            --bind \
            --platform "ubuntu-22.04=$RUNNER_IMAGE" \
            --pull=false
    ) || job_status=$?
    ((job_status == 0)) || status=$job_status
done

step "Cleaning ufeel-act/ (./build.sh clean)"
clean_copy

if ((status == 0)); then
    step "CI passed"
else
    step "CI failed (act exit code $status)"
fi
exit "$status"
