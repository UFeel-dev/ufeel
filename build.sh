#!/usr/bin/env bash

set -e

BUILD_DIR="build"
CMAKE_ARGS=("${@:2}")
CSHARP_PROJECT="csharp"

build()
{
    echo "Cleaning build..."
    rm -rf $BUILD_DIR

    echo "Configuring CMake..."
    mkdir -p $BUILD_DIR
    cd $BUILD_DIR
    cmake .. "${CMAKE_ARGS[@]}"
    cd ..
}

compile()
{
    echo "Compiling..."
    cd $BUILD_DIR
    make -j8    #just for the devlopment phase, will change when creating a pr
    cd ..
}

run()
{
    echo "Running C# test..."
    dotnet run --project $CSHARP_PROJECT
}

case "$1" in

build)
    build
    ;;

compile)
    compile
    ;;

run)
    run
    ;;

all)
    build
    compile
    run
    ;;
*)
    echo "Usage:"
    echo "  ./build.sh build"
    echo "  ./build.sh compile"
    echo "  ./build.sh run"
    echo "  ./build.sh all"
    exit 1
    ;;

esac
