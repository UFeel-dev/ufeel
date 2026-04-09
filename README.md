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

   * Compiles the native sources into a library
   * Moves the required artifacts into the C# project folder

3. **Run**
   Starts the C# test application.

## Requirements

* Linux (Ubuntu recommended)
* `dotnet` installed
* `clang` or `gcc`
* `cmake`

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