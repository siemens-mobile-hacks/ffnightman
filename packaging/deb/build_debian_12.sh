#!/bin/env bash

if [ -z $1 ]
then
    echo "Specify build directory"
    exit -1
fi;

if [ -z $2 ]
then
    echo "Specify build type"
    exit -2
fi;

BUILD_DEV="FALSE"

if [ ! "$2" = "dev" ] && [ ! "$2" = "prod" ]
then
    echo "Build type must be 'dev' or 'prod'"
    exit -3
fi

if [ "$2" = "dev" ]
then
    BUILD_DEV="TRUE"
fi

set -e

BUILD_DIR=$1

mkdir "deps_$BUILD_DIR"
cd "deps_$BUILD_DIR"

git clone https://github.com/siemens-mobile-hacks/libffshit.git
cd libffshit

if [ "$BUILD_DEV" = FALSE ]
then
    LIBFFSHIT_LATEST_TAG=$(git describe --tags --abbrev=0)

    echo "checkout $LIBFFSHIT_LATEST_TAG"
    git checkout ${LIBFFSHIT_LATEST_TAG}

    LIBFFSHIT_GIT_HASH=$(git rev-parse --short HEAD)
    LIBFFSHIT_VERSION_NUMBER=`echo "$LIBFFSHIT_LATEST_TAG" | sed "s/v//"`
    LIBFFSHIT_VERSION_STRING="$LIBFFSHIT_VERSION_NUMBER-$LIBFFSHIT_GIT_HASH"
else
    LIBFFSHIT_LATEST_TAG=$(git describe --tags --abbrev=0)
    LIBFFSHIT_GIT_HASH=$(git rev-parse --short HEAD)
    LIBFFSHIT_VERSION_NUMBER=`echo "$LIBFFSHIT_LATEST_TAG" | sed "s/v//"`
    LIBFFSHIT_VERSION_STRING="$LIBFFSHIT_VERSION_NUMBER-$LIBFFSHIT_GIT_HASH-unstable"
fi

cmake -DBUILD_DEV=$BUILD_DEV    -DCMAKE_BUILD_TYPE=Release \
                                -DBUILD_DEB_PACKAGE=TRUE \
                                -DDEB_DIST_NAME="debian-12" \
                                -DDEB_DIST_DEPS="libfmt9,libfmt-dev" \
                                -DDEB_DIST_ARCH="amd64" \
                                -B build

cmake --build build --config Release
cd build
cpack -G DEB
cd ../_packages_deb
sudo dpkg -i *.deb

cd ../../../
cmake -DBUILD_DEV=$BUILD_DEV    -DCMAKE_BUILD_TYPE=Release \
                                -DBUILD_DEB_PACKAGE=TRUE \
                                -DDEB_DIST_NAME="debian-12" \
                                -DDEB_DIST_DEPS="libfmt9,libfmt-dev,libspdlog1.10,libspdlog-dev,catch2,libffshit (= $LIBFFSHIT_VERSION_STRING)" \
                                -DDEB_DIST_ARCH="amd64" \
                                -B $BUILD_DIR

cmake --build $BUILD_DIR --config Release
