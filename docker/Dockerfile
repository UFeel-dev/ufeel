FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y \
        software-properties-common \
        wget \
        ca-certificates \
        lsb-release

RUN add-apt-repository ppa:dotnet/backports && \
    apt-get update

RUN wget https://apt.llvm.org/llvm.sh && \
    chmod +x llvm.sh && \
    ./llvm.sh 22 && \
    rm llvm.sh

RUN apt-get update && \
    apt-get install -y \
        git \
        git-lfs \
        dotnet-sdk-10.0 \
        clang-22 \
        clang-tidy-22 \
        clang-format-22 \
        llvm-22 \
        ninja-build \
        libasound2 \
        libfontconfig1 \
        libice6 \
        libsm6 \
        libx11-6 \
        xvfb \
        patchelf \
        iwyu \
        libcurl4-openssl-dev \
        libwayland-client0 \
        libwayland-cursor0 \
        libwayland-egl1 \
        libxkbcommon0 \
        && \
    rm -rf /var/lib/apt/lists/*

ARG CMAKE_VERSION=3.31.6

RUN wget "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz" \
        -O /tmp/cmake.tar.gz && \
    tar -xzf /tmp/cmake.tar.gz -C /opt && \
    rm /tmp/cmake.tar.gz

ENV PATH="/opt/cmake-${CMAKE_VERSION}-linux-x86_64/bin:${PATH}"

RUN ln -s /usr/bin/clang-format-22 /usr/local/bin/clang-format && \
    ln -s /usr/bin/clang-tidy-22 /usr/local/bin/clang-tidy && \
    ln -s /usr/bin/run-clang-tidy-22 /usr/local/bin/run-clang-tidy

WORKDIR /mnt

# TODO: make generated files editable by the host
