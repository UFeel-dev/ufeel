FROM ubuntu:26.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get upgrade -y

RUN apt-get install -y \
    software-properties-common

RUN add-apt-repository ppa:dotnet/backports

RUN apt-get update && \
    apt-get install -y \
    git \
    git-lfs \
    dotnet-sdk-8.0 \
    clang \
    clang-tidy-22 \
    clang-format-22 \
    cmake \
    portaudio19-dev \
    libjpeg62 \
    libwebp7 \
    libpng16-16 \
    libtiff6 \
    libopenexr-3-1-30 \
    libdc1394-25 \
    libgtk-3-0 \
    libyaml-cpp-dev \
    iwyu && \
    rm -rf /var/lib/apt/lists/*

RUN ln -s /usr/bin/clang-format-22 /usr/local/bin/clang-format && \
    ln -s /usr/bin/clang-tidy-22 /usr/local/bin/clang-tidy && \
    ln -s /usr/bin/run-clang-tidy-22 /usr/local/bin/run-clang-tidy

WORKDIR /mnt
