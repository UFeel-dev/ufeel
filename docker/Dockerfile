FROM ubuntu:22.04 AS ci

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y \
        ca-certificates \
        wget \
        gnupg \
        lsb-release && \
    wget -qO /etc/apt/trusted.gpg.d/apt.llvm.org.asc https://apt.llvm.org/llvm-snapshot.gpg.key && \
    echo "deb https://apt.llvm.org/jammy/ llvm-toolchain-jammy-22 main" \
        > /etc/apt/sources.list.d/llvm-22.list && \
    apt-get update && \
    apt-get install -y \
        clang-22 \
        clang-tidy-22 \
        clang-format-22 \
        ccache \
        git \
        git-lfs \
        ninja-build \
        patchelf \
        libasound2 \
        libcurl4-openssl-dev \
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

ARG CTCACHE_COMMIT=e33c063ca6e52b48bcefbc45d46d8c150ffa81ca

RUN mkdir -p /opt/ctcache && \
    wget -qO /opt/ctcache/clang_tidy_cache.py \
        "https://raw.githubusercontent.com/matus-chochlik/ctcache/${CTCACHE_COMMIT}/src/ctcache/clang_tidy_cache.py" && \
    printf '#!/bin/sh\nexec python3 /opt/ctcache/clang_tidy_cache.py /usr/bin/clang-tidy-22 "$@"\n' \
        > /usr/local/bin/clang-tidy-cache && \
    chmod +x /usr/local/bin/clang-tidy-cache

WORKDIR /mnt

FROM ci AS iwyu

ARG IWYU_VERSION=0.26

RUN apt-get update && \
    apt-get install -y llvm-22-dev libclang-22-dev && \
    rm -rf /var/lib/apt/lists/* && \
    git clone --depth 1 --branch "${IWYU_VERSION}" \
        https://github.com/include-what-you-use/include-what-you-use.git /tmp/iwyu && \
    cmake -S /tmp/iwyu -B /tmp/iwyu/build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_C_COMPILER=clang-22 \
        -DCMAKE_CXX_COMPILER=clang++-22 \
        -DCMAKE_PREFIX_PATH=/usr/lib/llvm-22 \
        -DCMAKE_INSTALL_PREFIX=/opt/iwyu && \
    cmake --build /tmp/iwyu/build && \
    cmake --install /tmp/iwyu/build

FROM ci AS dev

COPY --from=iwyu /opt/iwyu /usr/local

RUN apt-get update && \
    apt-get install -y software-properties-common && \
    add-apt-repository ppa:dotnet/backports && \
    apt-get update && \
    apt-get install -y \
        dotnet-sdk-10.0 \
        llvm-22 \
        clangd-22 \
        lldb-22 \
        lld-22 \
        libfontconfig1 \
        libice6 \
        libsm6 \
        libx11-6 \
        xvfb \
        libwayland-client0 \
        libwayland-cursor0 \
        libwayland-egl1 \
        libxkbcommon0 \
        && \
    rm -rf /var/lib/apt/lists/*

# TODO: make generated files editable by the host
