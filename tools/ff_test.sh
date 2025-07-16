#!/bin/env bash

if [ -z $1 ]
then
    echo "Specify FF test directory"
    exit -1
fi;

set -e

FF_TEST_DIR_PATH=$1

SCRIPT_PATH=${BASH_SOURCE[0]};
SCRIPT_PATH_FULL=`realpath "${SCRIPT_PATH}"`
SCRIPT_DIR=`dirname "$SCRIPT_PATH_FULL"`
NIGHTMAN_PATH="$SCRIPT_DIR/../build/ffnightman"

if [ ! -f ${NIGHTMAN_PATH} ]; then
    echo "ffnightman not found"

    exit -1
fi

cd ${FF_TEST_DIR_PATH}

FILES=$(find . -maxdepth 1 -iname '*.bin')

for FILE in $FILES
do
	echo Processing "$FILE"
	$NIGHTMAN_PATH -l -o "$FILE"
done
