#!/usr/bin/env bash

if ! xhost | grep -q "LOCAL:"; then
    xhost +local:docker &> /dev/null
fi

AUDIO_GID="$(getent group audio | cut -d: -f3)"

AUDIO_DEVICES=()

for device in /dev/snd/*; do
    if [ -c "$device" ]; then
        AUDIO_DEVICES+=(--device="$device")
    fi
done

docker run -it \
    --net=host \
    -e DISPLAY="$DISPLAY" \
    -v /tmp/.X11-unix:/tmp/.X11-unix:ro \
    -v "$(pwd)":/mnt \
    --device=/dev/video0:/dev/video0 \
    --group-add "$AUDIO_GID" \
    "${AUDIO_DEVICES[@]}" \
    --entrypoint /bin/bash \
    ufeeldocker:latest \
    -c 'if ! getent group "$1" >/dev/null; then groupadd -g "$1" hostaudio; fi; exec bash' \
    _ "$AUDIO_GID"
