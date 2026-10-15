# Docker

> [<- Back to README](../README.md)

## Overview

You can run this project in a docker container so all dependencies are already installed.

## Installation

1. Setup the Dockerfile (the build command should be ran in the same directory as the Dockerfile)

```bash
cd docker
docker build -t ufeeldocker .
cd ..
```

1. Run the docker container from the root of the repository

```bash
./docker/run.sh
```
