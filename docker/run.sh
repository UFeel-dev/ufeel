#!/usr/bin/env bash

set -Eeuo pipefail

IMAGE="${UFEEL_DOCKER_IMAGE:-ufeeldocker:latest}"
DISPLAY_MODE="${DISPLAY_MODE:-auto}"
AUDIO_BACKEND="${AUDIO_BACKEND:-alsa}"

DOCKER_ARGS=()
RUN_ARGS=(-i)
DEVICE_GIDS=()
RUNTIME_DIRS=()

XHOST_ENABLED=0

add_gid()
{
    local gid="$1"

    [[ "$gid" =~ ^[0-9]+$ ]] || return 0

    for existing in "${DEVICE_GIDS[@]}"; do
        [[ "$existing" == "$gid" ]] && return 0
    done

    DEVICE_GIDS+=("$gid")
    DOCKER_ARGS+=(--group-add "$gid")
}

add_device()
{
    local device="$1"

    [[ -c "$device" ]] || return 0

    DOCKER_ARGS+=(--device="$device")
    add_gid "$(stat -c '%g' -- "$device")"
}

mount_runtime_dir()
{
    local runtime_dir="$1"

    [[ -d "$runtime_dir" ]] || {
        echo "Invalid runtime directory: $runtime_dir" >&2
        exit 1
    }

    for existing in "${RUNTIME_DIRS[@]}"; do
        [[ "$existing" == "$runtime_dir" ]] && return 0
    done

    RUNTIME_DIRS+=("$runtime_dir")
    DOCKER_ARGS+=(-v "$runtime_dir:$runtime_dir")
}

cleanup()
{
    if [[ "$XHOST_ENABLED" == 1 ]]; then
        xhost -SI:localuser:root >/dev/null 2>&1 || true
    fi
}

resolve_wayland()
{
    local display="${WAYLAND_DISPLAY:-}"
    local runtime="${XDG_RUNTIME_DIR:-}"

    [[ -n "$display" ]] || return 1

    if [[ "$display" = /* ]]; then
        WAYLAND_SOCKET="$display"
        WAYLAND_RUNTIME_DIR="$(dirname "$display")"
        WAYLAND_DISPLAY_NAME="$(basename "$display")"
        WAYLAND_XDG_RUNTIME_DIR="${runtime:-$WAYLAND_RUNTIME_DIR}"
    else
        [[ -n "$runtime" ]] || return 1

        WAYLAND_RUNTIME_DIR="$runtime"
        WAYLAND_SOCKET="$runtime/$display"
        WAYLAND_DISPLAY_NAME="$display"
        WAYLAND_XDG_RUNTIME_DIR="$runtime"
    fi

    [[ -S "$WAYLAND_SOCKET" ]]
}

configure_wayland()
{
    mount_runtime_dir "$WAYLAND_RUNTIME_DIR"

    if [[ "$WAYLAND_XDG_RUNTIME_DIR" != "$WAYLAND_RUNTIME_DIR" ]]; then
        mount_runtime_dir "$WAYLAND_XDG_RUNTIME_DIR"
    fi

    DOCKER_ARGS+=(
        -e WAYLAND_DISPLAY="$WAYLAND_SOCKET"
        -e XDG_RUNTIME_DIR="$WAYLAND_XDG_RUNTIME_DIR"
    )

    add_gid "$(stat -c '%g' -- "$WAYLAND_SOCKET")"
}

configure_x11()
{
    if [[ -z "${DISPLAY:-}" ]]; then
        echo "DISPLAY is not set." >&2
        return 1
    fi

    if [[ ! -d /tmp/.X11-unix ]]; then
        echo "X11 socket directory not found." >&2
        return 1
    fi

    if ! command -v xhost >/dev/null 2>&1; then
        echo "xhost is required for X11 access." >&2
        return 1
    fi

    if ! xhost +SI:localuser:root >/dev/null; then
        echo "Could not grant X11 access to the container." >&2
        return 1
    fi

    XHOST_ENABLED=1

    DOCKER_ARGS+=(
        -e DISPLAY="$DISPLAY"
        -v /tmp/.X11-unix:/tmp/.X11-unix:ro
    )
}

trap cleanup EXIT

DOCKER_ARGS+=(
    -v "$(pwd -P):/mnt"
    -w /mnt
)

DOCKER_NETWORK="${UFEEL_DOCKER_NETWORK:-host}"
DOCKER_ARGS+=(--network "$DOCKER_NETWORK")

if [[ -n "${UFEEL_DOCKER_GPUS:-}" ]]; then
    DOCKER_ARGS+=(--gpus "$UFEEL_DOCKER_GPUS")
fi

case "$DISPLAY_MODE" in
    auto)
        WAYLAND_AVAILABLE=0
        X11_AVAILABLE=0

        if resolve_wayland; then
            WAYLAND_AVAILABLE=1
        fi

        if [[ -n "${DISPLAY:-}" && -d /tmp/.X11-unix ]]; then
            X11_AVAILABLE=1
        fi

        if [[ "$WAYLAND_AVAILABLE" == 1 ]]; then
            configure_wayland
        fi

        if [[ "$X11_AVAILABLE" == 1 ]]; then
            if ! configure_x11; then
                if [[ "$WAYLAND_AVAILABLE" == 0 ]]; then
                    exit 1
                fi

                echo "Warning: X11 is unavailable, continuing with Wayland." >&2
            fi
        fi

        if [[ "$WAYLAND_AVAILABLE" == 0 && "$X11_AVAILABLE" == 0 ]]; then
            echo "Running without a graphical display."
        fi
        ;;

    wayland)
        if ! resolve_wayland; then
            echo "A valid Wayland socket could not be found." >&2
            exit 1
        fi

        configure_wayland
        ;;

    x11)
        configure_x11
        ;;

    headless)
        echo "Running without a graphical display."
        ;;

    *)
        echo "Invalid DISPLAY_MODE: $DISPLAY_MODE" >&2
        exit 1
        ;;
esac

case "$AUDIO_BACKEND" in
    alsa)
        if [[ -d /dev/snd ]]; then
            for device in /dev/snd/*; do
                add_device "$device"
            done
        fi
        ;;

    pipewire)
        RUNTIME_DIR="${XDG_RUNTIME_DIR:-}"

        if [[ -z "$RUNTIME_DIR" || ! -S "$RUNTIME_DIR/pipewire-0" ]]; then
            echo "PipeWire socket not found." >&2
            exit 1
        fi

        mount_runtime_dir "$RUNTIME_DIR"

        DOCKER_ARGS+=(
            -e XDG_RUNTIME_DIR="$RUNTIME_DIR"
            -e PIPEWIRE_REMOTE=pipewire-0
        )
        ;;

    pulse)
        RUNTIME_DIR="${XDG_RUNTIME_DIR:-}"
        PULSE_SOCKET="$RUNTIME_DIR/pulse/native"

        if [[ -z "$RUNTIME_DIR" || ! -S "$PULSE_SOCKET" ]]; then
            echo "PulseAudio socket not found." >&2
            exit 1
        fi

        mount_runtime_dir "$RUNTIME_DIR"

        DOCKER_ARGS+=(
            -e XDG_RUNTIME_DIR="$RUNTIME_DIR"
            -e PULSE_SERVER="unix:$PULSE_SOCKET"
        )
        ;;

    none)
        ;;

    *)
        echo "Invalid AUDIO_BACKEND: $AUDIO_BACKEND" >&2
        exit 1
        ;;
esac

for device in /dev/video* /dev/media* /dev/v4l-subdev*; do
    add_device "$device"
done

if [[ -d /dev/dri ]]; then
    for device in /dev/dri/*; do
        add_device "$device"
    done
fi

if [[ "${UFEEL_USB_DEVICES:-0}" == 1 && -d /dev/bus/usb ]]; then
    for device in /dev/bus/usb/*/*; do
        add_device "$device"
    done
fi

if [[ -t 0 && -t 1 ]]; then
    RUN_ARGS+=(-t)
fi

DEVICE_GIDS_STRING="${DEVICE_GIDS[*]:-}"

docker run \
    --rm \
    "${RUN_ARGS[@]}" \
    "${DOCKER_ARGS[@]}" \
    -e UFEEL_DEVICE_GIDS="$DEVICE_GIDS_STRING" \
    --entrypoint /bin/bash \
    "$IMAGE" \
    -lc '
        read -r -a gids <<< "${UFEEL_DEVICE_GIDS:-}"

        for gid in "${gids[@]}"; do
            if ! getent group "$gid" >/dev/null; then
                groupadd --gid "$gid" "hostgroup_$gid"
            fi
        done

        if (($#)); then
            exec "$@"
        else
            exec bash
        fi
    ' \
    -- "$@"
