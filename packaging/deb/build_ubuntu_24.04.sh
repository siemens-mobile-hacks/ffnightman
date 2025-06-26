#!/bin/env bash

if [ -z $1 ]
then
    echo "Specify build directory"
    exit -1
fi;

set -e

BUILD_DIR=$1
ROOT_DIR=$(pwd)

mkdir "deps_$BUILD_DIR"
cd "deps_$BUILD_DIR"
git clone https://github.com/siemens-mobile-hacks/libffshit.git
cd libffshit
LIBFFSHIT_LATEST_TAG=$(git describe --tags)
LIBFFSHIT_GIT_HASH=$(git rev-parse --short HEAD)
LIBFFSHIT_VERSION_NUMBER=`echo "$LIBFFSHIT_LATEST_TAG" | sed "s/v//"`
LIBFFSHIT_VERSION_STRIING="$LIBFFSHIT_VERSION_NUMBER-$LIBFFSHIT_GIT_HASH"

echo $LIBFFSHIT_VERSION_STRIING

echo "checkout $LIBFFSHIT_LATEST_TAG"
git checkout ${LIBFFSHIT_LATEST_TAG}
cmake -DCMAKE_BUILD_TYPE=Release -DDIST_NAME="ubuntu-24.04" -DDIST_DEPS="libfmt9,libfmt-dev" -DDIST_ARCH="amd64" -B build
cmake --build build --config Release
cd build
cpack -G DEB
cd ../_packages_deb
sudo dpkg -i *.deb

cd "$ROOT_DIR"
cmake -DCMAKE_BUILD_TYPE=Release  -DDIST_NAME="ubuntu-24.04" -DDIST_DEPS="libfmt9,libfmt-dev,libspdlog1.12,libspdlog-dev,catch2,libffshit (= $LIBFFSHIT_VERSION_STRIING)" -DDIST_ARCH="amd64"  -B $BUILD_DIR
cmake --build $BUILD_DIR --config Release
