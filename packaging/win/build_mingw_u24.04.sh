#!/bin/env bash

ROOT_DIR=`pwd`

apt update
apt -y install mingw-w64 mingw-w64-tools mingw-w64-i686-dev mingw-w64-x86-64-dev mingw-w64-common win-iconv-mingw-w64-dev cmake git make

git clone https://github.com/siemens-mobile-hacks/libffshit.git
cd libffshit
cd ..

git clone https://github.com/fmtlib/fmt.git
cd fmt
git checkout 9.1.0
cmake -DCMAKE_TOOLCHAIN_FILE="$ROOT_DIR/libffshit/packaging/win/mingw64.cmake" -DCMAKE_BUILD_TYPE=Release -DFMT_DOC=FALSE -DFMT_TEST=FALSE -B build_win
cmake --build build_win --config Release
cmake --install build_win --prefix target_win
cd ..

git clone https://github.com/gabime/spdlog.git
cd spdlog
git checkout v1.12.0
cmake -DCMAKE_TOOLCHAIN_FILE="$ROOT_DIR/libffshit/packaging/win/mingw64.cmake" -DCMAKE_BUILD_TYPE=Release -DSPDLOG_BUILD_SHARED=OFF -DSPDLOG_BUILD_EXAMPLE=OFF -B build_win
cmake --build build_win --config Release
cmake --install build_win --prefix target_win
cd ..

cd libffshit
cmake -DCMAKE_TOOLCHAIN_FILE="$ROOT_DIR/libffshit/packaging/win/mingw64.cmake" -DCMAKE_PREFIX_PATH="$ROOT_DIR/fmt/target_win" -DCMAKE_BUILD_TYPE=Release -B build_win
cmake --build build_win --config Release
cmake --install build_win --prefix target_win
cd ..

git clone https://github.com/siemens-mobile-hacks/ffnightman.git
cd ffnightman
cmake -DCMAKE_TOOLCHAIN_FILE="$ROOT_DIR/libffshit/packaging/win/mingw64.cmake" -DCMAKE_PREFIX_PATH="$ROOT_DIR/fmt/target_win;$ROOT_DIR/spdlog/target_win;$ROOT_DIR/libffshit/target_win" -DCMAKE_BUILD_TYPE=Release -B build_win
cmake --build build_win --config Release
cmake --install build_win --prefix target_win
