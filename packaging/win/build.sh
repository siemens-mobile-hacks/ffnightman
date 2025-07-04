#!/bin/env bash

# Setup variables

BUILD_DEV="FALSE"

if [ -z $1 ]
then
    echo "Specify build type"
    exit -1
fi;

if [ ! "$1" = "dev" ] && [ ! "$1" = "prod" ]
then
    echo "Build type must be 'dev' or 'prod'"
    exit -2
fi

if [ "$1" = "dev" ]
then
    BUILD_DEV="TRUE"
fi

set -e

BUILD_ROOT_DIR=$(pwd)
DEPS_DIR="$BUILD_ROOT_DIR/deps"

# ==== Building dependencies =====

mkdir deps
cd deps

# ==== Cloning libffshit =====

git clone https://github.com/siemens-mobile-hacks/libffshit.git
cd libffshit

LIBFFSHIT_LATEST_TAG=$(git describe --tags --abbrev=0)

if [ "$BUILD_DEV" = FALSE ]
then
    git checkout ${LIBFFSHIT_LATEST_TAG}
fi

cd ..

# ==== Building libfmt ====

git clone https://github.com/fmtlib/fmt.git
cd fmt
git checkout 9.1.0
cmake -DCMAKE_TOOLCHAIN_FILE="$DEPS_DIR/libffshit/packaging/win/mingw64.cmake" -DCMAKE_BUILD_TYPE=Release -DFMT_DOC=FALSE -DFMT_TEST=FALSE -B build_win
cmake --build build_win --config Release
cmake --install build_win --prefix target_win
cd ..

# ==== Building spdlog ====

git clone https://github.com/gabime/spdlog.git
cd spdlog
git checkout v1.12.0
cmake -DCMAKE_TOOLCHAIN_FILE="$DEPS_DIR/libffshit/packaging/win/mingw64.cmake" -DCMAKE_BUILD_TYPE=Release -DSPDLOG_BUILD_SHARED=OFF -DSPDLOG_BUILD_EXAMPLE=OFF -B build_win
cmake --build build_win --config Release
cmake --install build_win --prefix target_win
cd ..

# ==== Building libffshit ====

cd libffshit
cmake -DBUILD_DEV=$BUILD_DEV    -DCMAKE_BUILD_TYPE=Release \
                                -DCMAKE_TOOLCHAIN_FILE="$DEPS_DIR/libffshit/packaging/win/mingw64.cmake" \
                                -DCMAKE_PREFIX_PATH="$DEPS_DIR/fmt/target_win" \
                                -B build_win

cmake --build build_win --config Release
cmake --install build_win --prefix target_win

# ==== Building ffnightman ====

cd "$BUILD_ROOT_DIR"
cmake -DBUILD_DEV=$BUILD_DEV    -DCMAKE_BUILD_TYPE=Release \
                                -DCMAKE_TOOLCHAIN_FILE="$DEPS_DIR/libffshit/packaging/win/mingw64.cmake" \
                                -DCMAKE_PREFIX_PATH="$DEPS_DIR/fmt/target_win;$DEPS_DIR/spdlog/target_win;$DEPS_DIR/libffshit/target_win" \
                                -B build_win

cmake --build build_win --config Release
cmake --install build_win --prefix target_win

FFNIGHTMAN_LATEST_TAG=$(git describe --tags --abbrev=0)
FFNIGHTMAN_GIT_HASH=$(git rev-parse --short HEAD)
FFNIGHTMAN_VERSION_STRING="$FFNIGHTMAN_LATEST_TAG-$FFNIGHTMAN_GIT_HASH"

cd target_win/bin

if [ "$BUILD_DEV" = TRUE ]
then
    FFNIGHTMAN_VERSION_STRING="$FFNIGHTMAN_VERSION_STRING-unstable"
fi

FFNIGHTMAN_EXE_NAME="ffnightman-$FFNIGHTMAN_VERSION_STRING.exe"

mv ffnightman.exe "$FFNIGHTMAN_EXE_NAME"

zip -r "ffnightman-windows-x64-$FFNIGHTMAN_VERSION_STRING.zip" "$FFNIGHTMAN_EXE_NAME"
