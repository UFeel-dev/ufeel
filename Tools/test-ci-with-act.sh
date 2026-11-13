#!/usr/bin/env bash

set -euo pipefail

readonly ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
readonly COPY_DIR="$ROOT_DIR/ufeel-act"
readonly RUNNER_IMAGE="catthehacker/ubuntu:act-22.04"
readonly DEFAULT_JOBS=(lint build)

error()
{
    printf 'error: %s\n' "$*" >&2
    exit 1
}

usage()
{
    printf 'usage: %s [lint|build|release]...\n' "${0##*/}"
    printf 'Runs the given CI jobs with act (default: %s).\n' "${DEFAULT_JOBS[*]}"
    printf 'release runs as a push to the release branch and skips publishing.\n'
}

https_url()
{
    local url="$1"
    if [[ "$url" =~ ^git@([^:]+):(.+)$ || "$url" =~ ^ssh://git@([^/]+)/(.+)$ ]]; then
        url="https://${BASH_REMATCH[1]}/${BASH_REMATCH[2]}"
    fi
    printf '%s\n' "$url"
}

step()
{
    printf '\n==> %s\n' "$*"
}

clean_copy()
{
    [[ -d "$COPY_DIR" ]] || return 0
    docker run --rm -v "$COPY_DIR:/mnt" -w /mnt "$RUNNER_IMAGE" \
        bash -c "if [[ -f build.sh ]]; then ./build.sh clean; fi; chown -R $(id -u):$(id -g) /mnt"
}

copy_working_tree()
{
    local lfs_pointers path
    lfs_pointers="$(comm -23 <(git lfs ls-files --name-only | sort) <(git ls-files --modified | sort))"
    { git ls-files -z --recurse-submodules; git ls-files -z --others --exclude-standard; } |
        grep -zvxFf <(printf '%s\n' "$lfs_pointers") |
        tar --null --files-from=- --ignore-failed-read -cf - | tar -xf - -C "$COPY_DIR"
    while IFS= read -r path; do
        [[ -n "$path" ]] || continue
        mkdir -p "$COPY_DIR/$(dirname -- "$path")"
        git cat-file blob ":$path" >"$COPY_DIR/$path"
        chmod --reference="$path" "$COPY_DIR/$path"
    done <<<"$lfs_pointers"
}

seed_lfs_objects()
{
    local source target oid object
    source="$(git rev-parse --path-format=absolute --git-common-dir)/lfs/objects"
    target="$COPY_DIR/.git/lfs/objects"
    while read -r oid _; do
        object="${oid:0:2}/${oid:2:2}/$oid"
        [[ -f "$source/$object" ]] || continue
        mkdir -p "$(dirname -- "$target/$object")"
        ln "$source/$object" "$target/$object" 2>/dev/null || cp "$source/$object" "$target/$object"
    done < <(git lfs ls-files --long --include='3rdparty/**')
}

jobs=("$@")
((${#jobs[@]})) || jobs=("${DEFAULT_JOBS[@]}")
for job in "${jobs[@]}"; do
    case "$job" in
        lint|build|release) ;;
        -h|--help) usage; exit 0 ;;
        *) usage >&2; error "unknown job '$job'" ;;
    esac
done

for tool in act docker git tar; do
    command -v "$tool" >/dev/null 2>&1 || error "required command '$tool' is not installed"
done
git lfs version >/dev/null 2>&1 || error "required command 'git lfs' is not installed"

if ! docker image inspect "$RUNNER_IMAGE" >/dev/null 2>&1; then
    step "Pulling the runner image $RUNNER_IMAGE"
    docker pull "$RUNNER_IMAGE"
fi

step "Removing the previous ufeel-act/"
clean_copy
rm -rf "$COPY_DIR" || error "cannot remove $COPY_DIR, check its ownership"

cd "$ROOT_DIR"

step "Copying the working tree to ufeel-act/"
mkdir "$COPY_DIR"
copy_working_tree

branch="$(git rev-parse --abbrev-ref HEAD)"
[[ "$branch" != HEAD ]] || branch="ci-act-test"
git -C "$COPY_DIR" init --quiet --initial-branch="$branch"
if origin="$(git remote get-url origin 2>/dev/null)"; then
    git -C "$COPY_DIR" remote add origin "$(https_url "$origin")"
fi
seed_lfs_objects
git -C "$COPY_DIR" add --all --force
git -C "$COPY_DIR" -c user.name=act -c user.email=act@localhost \
    commit --quiet --message "Local CI test"

pull_request_event="$(mktemp)"
push_event="$(mktemp)"
trap 'rm -f "$pull_request_event" "$push_event"' EXIT
printf '{ "pull_request": { "base": { "ref": "main" }, "head": { "ref": "%s" } } }\n' "$branch" \
    >"$pull_request_event"
printf '{ "ref": "refs/heads/release" }\n' >"$push_event"

status=0
for job in "${jobs[@]}"; do
    if [[ "$job" == release ]]; then
        event=push
        event_file="$push_event"
        step "Running the CI job '$job' with act (push to release, publishing skipped)"
    else
        event=pull_request
        event_file="$pull_request_event"
        step "Running the CI job '$job' with act (pull_request to main)"
    fi
    job_status=0
    (
        cd "$COPY_DIR"
        act "$event" \
            --workflows .github/workflows \
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
