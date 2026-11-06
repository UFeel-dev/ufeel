# Docker

> [<- Back to README](../README.md)

## Overview

You can run this project in a docker container so all dependencies are already installed.

## Installation

1. Setup the Dockerfile (the build command should be ran in the same directory as the Dockerfile)

```bash
cd docker
docker build --pull --no-cache -t ufeeldocker:latest .
cd ..
```

1. Run the docker container from the root of the repository

```bash
./docker/run.sh
```

## Running Script

Here are some options for the `run.sh` script:

### Automatic display detection, ALSA audio

./docker/run.sh

### Force Wayland

DISPLAY_MODE=wayland ./docker/run.sh

### Force X11

DISPLAY_MODE=x11 ./docker/run.sh

### Headless build

DISPLAY_MODE=headless ./docker/run.sh ./build.sh all

### Use host networking

UFEEL_DOCKER_NETWORK=host ./docker/run.sh

### Expose NVIDIA GPUs (requires NVIDIA Container Toolkit)

UFEEL_DOCKER_GPUS=all ./docker/run.sh
