# UFEEL Library

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

- Linux (Ubuntu recommended)
- `dotnet` installed (8.0)
- `clang` or `gcc`
- `cmake`
- `portaudio` (dev)

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