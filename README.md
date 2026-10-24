# UFEEL Library

[![Discord](https://dcbadge.limes.pink/api/server/https://discord.gg/TxYTDXMJkM)](https://discord.gg/TxYTDXMJkM)

[![Discord](https://img.shields.io/discord/1461581735631523900.svg)](https://discord.gg/TxYTDXMJkM)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](./LICENSE)
![Unity](https://img.shields.io/badge/Unity-%23000000.svg?logo=unity&logoColor=white)
![Linux](https://img.shields.io/badge/Linux-FCC624?logo=linux&logoColor=black)

![C++](https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![C#](https://img.shields.io/badge/c%23-%23239120.svg?style=for-the-badge&logo=csharp&logoColor=white)![Espressif](https://img.shields.io/badge/espressif-E7352C.svg?style=for-the-badge&logo=espressif&logoColor=white)

This project provides the **UFEEL native library**, a tool to handle emotion recognition, eye tracking, speech to text and heart rate sensing.

## Quick Start

1. Make sure you have git lfs installed and setup. If not, refer to the [instructions to install Git LFS](#download-third-party-libraries).

2. Clone the [repository](https://github.com/UFeel-dev/ufeel).

3. Setup the docker environment: [Docker-based setup documentation](docker/README.md) (or install dependencies).

4. Use the `build.sh` script to build and run the project:

```bash
./build.sh all
```

Note: you will need appropriate input devices to test some of the features, such as a camera and a microphone.

To use clang-related tools (e.g. linting, formatting, or even some IDE features), you should setup the compile database:

- Install `bear`.
- Remove the `build` directory if it exists.
- Run `bear -- ./build.sh all`.

This is because the compile database auto-generated from `cmake` is not always usable as-is.

## Requirements

Docker installation:

- Docker >= 20.0

Manual installation:

- Linux
- GLIBC >= 2.36
- `dotnet` == 10.0
- `clang` >= 22.0
- `cmake` >= 3.20
- `ninja-build` >= 1.13.2
- `git-lfs` >= 3.4.1
- `clang-tidy` >= 22.0
- `clang-format` >= 22.0
- `iwyu` >= 0.26

In case some dependencies are missing for your system, we would be grateful if you can open an appropriate issue. You might be able to find some of the dependencies in the [Dockerfile](docker/Dockerfile).

## Third-Party Libraries (Git LFS Required)

This project uses **Git LFS** to store large third-party libraries located in the `3rdparty/` folder (e.g. `.so`, `.a` files).

After cloning the repository, you may need to install Git LFS and pull the required files.

### Install Git LFS (Ubuntu / Debian)

```bash
sudo apt update
sudo apt install git-lfs
```

Then enable it:

```bash
git lfs install
```

### Download third-party libraries

After cloning the repository, run:

```bash
git lfs pull
```

This will download all required libraries inside the `3rdparty/` folder.

### Full setup example

```bash
git clone https://github.com/UFeel-dev/ufeel
git lfs install
git lfs pull
```

You can now build the project normally.

## Project Structure

```
.
├── build.sh
├── csharp/        # C# test project
├── Library/       # native sources
├── Wrapper/       # C++ Wrapper
├── 3rdparty/      # external dependencies
├── build/         # generated (created by script)
└── docker/        # Docker environment
```

## Manual build (optional)

If you prefer running steps manually:

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
