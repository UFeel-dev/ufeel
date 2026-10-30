# UFEEL Library

[![Discord](https://dcbadge.limes.pink/api/server/https://discord.gg/TxYTDXMJkM)](https://discord.gg/TxYTDXMJkM)

[![Discord](https://img.shields.io/discord/1461581735631523900.svg)](https://discord.gg/TxYTDXMJkM)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](./LICENSE)
![Unity](https://img.shields.io/badge/Unity-%23000000.svg?logo=unity&logoColor=white)
![Linux](https://img.shields.io/badge/Linux-FCC624?logo=linux&logoColor=black)

![C++](https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![C#](https://img.shields.io/badge/c%23-%23239120.svg?style=for-the-badge&logo=csharp&logoColor=white)
![Espressif](https://img.shields.io/badge/espressif-E7352C.svg?style=for-the-badge&logo=espressif&logoColor=white)

This project provides the **UFEEL native library**, a tool to handle emotion recognition, eye tracking, speech to text and heart rate sensing.

## Quick Start

1. Install and configure Git LFS. See [Third-Party Libraries](#third-party-libraries).
2. Clone the repository.
3. Initialize the third-party submodules and download Git LFS files.
4. Set up Docker using the [Docker documentation](docker/README.md).
5. Build and run the project:

```bash
git clone https://github.com/UFeel-dev/ufeel
cd ufeel

git lfs install
git submodule update --init --recursive
git lfs pull

cd docker
docker build -t ufeeldocker .
cd ..
./docker/run.sh

./build.sh all
```

Docker is the recommended build environment. It provides a reproducible toolchain and avoids requiring the native build dependencies on the host.

Note: you will need appropriate input devices to test some features, such as a camera and a microphone.

To use clang-related tools (e.g. linting, formatting, or IDE features), set up the compilation database:

- Install `bear`.
- Remove the `build` directory if it exists.
- Run:

```bash
bear -- ./build.sh all
```

The generated compilation database from CMake is not always directly usable by all tooling.

## Requirements

### Docker build

- Docker >= 20.0
- Git
- Git LFS >= 3.4.1

### Manual build

Manual builds are supported but require compatible versions of all third-party dependencies.

- Linux x86-64
- GLIBC compatible with the build environment and bundled dependencies
- .NET SDK 10.0
- Clang >= 22.0
- CMake >= 3.20
- Ninja >= 1.13.2
- Git LFS >= 3.4.1
- clang-tidy >= 22.0
- clang-format >= 22.0
- patchelf
- IWYU >= 0.26

The native package still relies on some operating-system libraries, including glibc, libstdc++, audio libraries and other system dependencies.

If some dependencies are missing for your system, please open an issue. The [Dockerfile](docker/Dockerfile) documents the build environment.

## Third-Party Libraries

This project uses Git LFS to store large third-party libraries in `3rdparty/`, including prebuilt `.so` and `.a` files.

yaml-cpp is maintained as a Git submodule.

After cloning, initialize all submodules and download the Git LFS files:

```bash
git lfs install
git submodule update --init --recursive
git lfs pull
```

## Project Structure

```text
.
├── build.sh
├── csharp/        # C# test project
├── Library/       # Native sources
├── Wrapper/       # C++ wrapper
├── 3rdparty/      # External dependencies
├── build/         # Generated build files
└── docker/        # Docker environment
```

## Manual Build

If you prefer running the steps manually:

```bash
./build.sh configure
./build.sh build
./build.sh package
```

## Documentation

| Document | Description |
| --- | --- |
| [Contributing](CONTRIBUTING.md) | Contribution guidelines |
| [Disclaimer](DISCLAIMER.md) | Limitations and prohibited use |
| [Docker](docker/README.md) | Docker setup instructions |

## License

Apache 2.0 License - see [LICENSE](LICENSE).
