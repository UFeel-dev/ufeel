FROM ubuntu:26.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y \
    software-properties-common && \
    add-apt-repository ppa:dotnet/backports && \
    apt-get update && \
    apt-get install -y \
    git \
    git-lfs \
    dotnet-sdk-8.0 \
    clang-22 \
    clang++-22 \
    clang-tidy-22 \
    clang-format-22 \
    cmake \
    ninja-build \
    libasound2t64 \
    libyaml-cpp-dev \
    iwyu && \
    rm -rf /var/lib/apt/lists/*

RUN ln -s /usr/bin/clang-format-22 /usr/local/bin/clang-format && \
    ln -s /usr/bin/clang-tidy-22 /usr/local/bin/clang-tidy && \
    ln -s /usr/bin/run-clang-tidy-22 /usr/local/bin/run-clang-tidy

WORKDIR /mnt
