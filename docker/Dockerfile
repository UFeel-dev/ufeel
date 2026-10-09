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
        dotnet-sdk-8.0 \
        clang-22 \
        clang-tidy-22 \
        clang-format-22 \
        cmake \
        ninja-build \
        libasound2 \
        libfontconfig1 \
        libice6 \
        libsm6 \
        libyaml-cpp-dev \
        patchelf \
        iwyu && \
    rm -rf /var/lib/apt/lists/*

RUN ln -s /usr/bin/clang-format-22 /usr/local/bin/clang-format && \
    ln -s /usr/bin/clang-tidy-22 /usr/local/bin/clang-tidy && \
    ln -s /usr/bin/run-clang-tidy-22 /usr/local/bin/run-clang-tidy

WORKDIR /mnt

# TODO: make generated files editable by the host
