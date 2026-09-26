#!/bin/env bash

if ! xhost | grep -q "LOCAL:"; then
    xhost +local:docker &> /dev/null
fi

docker run -it \
    --net=host \
    -e DISPLAY=$DISPLAY \
    -v /tmp/.X11-unix:/tmp/.X11-unix:ro \
    -v .:/mnt \
    ufeeldocker:latest
