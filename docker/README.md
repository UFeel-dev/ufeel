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

The Dockerfile has two targets. The default one (`dev`, used above) contains everything: build
toolchain, .NET SDK for the integration tests, Xvfb/X11/Wayland libraries, coverage tools, clangd,
lldb, and include-what-you-use 0.26 (built from source against clang 22, because Ubuntu 22.04 only
ships 0.17). The `ci` target only contains what the CI needs to configure,
build, and lint the native library, which makes it much smaller:

```bash
docker build --target ci -t ufeeldocker:ci .
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
