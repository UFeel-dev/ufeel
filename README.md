# UFEEL Library

[![Discord](https://dcbadge.limes.pink/api/server/https://discord.gg/TxYTDXMJkM)](https://discord.gg/TxYTDXMJkM)

[![Discord](https://img.shields.io/discord/1461581735631523900.svg)](https://discord.gg/TxYTDXMJkM)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](./LICENSE)
![Unity](https://img.shields.io/badge/Unity-%23000000.svg?logo=unity&logoColor=white)
![Linux](https://img.shields.io/badge/Linux-FCC624?logo=linux&logoColor=black)

![C++](https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![C#](https://img.shields.io/badge/c%23-%23239120.svg?style=for-the-badge&logo=csharp&logoColor=white)![Espressif](https://img.shields.io/badge/espressif-E7352C.svg?style=for-the-badge&logo=espressif&logoColor=white)

This project provides the **UFEEL native library** with a C# test application.

## Quick Start

Use the `build.sh` script to build and run the project:

```bash
./build.sh all
```

## What the script does

The script performs the following steps:

1. **Build**
   Creates the `build/` directory.

2. **Compile**

   - Compiles the native sources into a library
   - Moves the required artifacts into the C# project folder

3. **Run**
   Starts the C# test application.

## Requirements

- Linux
- GLIBC >= 2.39
- `dotnet` installed == 8.0
- `clang` >= 14.0.6
- `cmake` >= 3.20
- `portaudio` >= 19.6.0
- `git-lfs` >= 3.4.1

## Third-Party Libraries (Git LFS Required)

This project uses **Git LFS** to store large third-party libraries located in the `3rdparty/` folder (e.g. `.so`, `.a` files).

After cloning the repository, you must install Git LFS and pull the required files.

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
git clone git@github.com:augustin-grosnon/ufeel.git
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
└── build/         # generated (created by script)
```

## Manual build (optional)

If you prefer running steps manually:

```bash
./build.sh build
./build.sh compile
./build.sh run
```