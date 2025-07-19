#!/bin/env bash

if [ -z $1 ]
then
    echo "Specify FF test directory"
    exit -1
fi;

if [ -z $2 ]
then
    echo "Specify platform (lowercase available):"
    echo "[ EGOLD, SGOLD, SGOLD2, SGOLD2_ELKA, ALL ]"
    exit -1
fi;

if [ -z $3 ]; then
    VALGRIND_CHECK=false
else
    if which valgrind 2>/dev/null; then
        echo "Valgrind check enabled"
    else
        echo "Valgrind not installed"
        exit -1
    fi

    VALGRIND_CHECK=true
fi

set -e

PLATFORM="${2,,}"

FF_TEST_DIR_PATH=`realpath "${1}"`

FF_TEST_DIR_EGOLD="$FF_TEST_DIR_PATH/EGOLD"
FF_TEST_DIR_SGOLD="$FF_TEST_DIR_PATH/SGOLD"
FF_TEST_DIR_SGOLD2="$FF_TEST_DIR_PATH/SGOLD2"
FF_TEST_DIR_SGOLD2_ELKA="$FF_TEST_DIR_PATH/SGOLD2_ELKA"

SCRIPT_PATH=${BASH_SOURCE[0]};
SCRIPT_PATH_FULL=`realpath "${SCRIPT_PATH}"`
SCRIPT_DIR=`dirname "$SCRIPT_PATH_FULL"`
NIGHTMAN_PATH="$SCRIPT_DIR/../build/ffnightman"

if [ ! -f ${NIGHTMAN_PATH} ]; then
    echo "ffnightman not found"

    exit -1
fi

check_dir() {
    FILES=$(find . -maxdepth 1 -iname '*.bin')

    for FILE in $FILES
    do
        echo Processing "$FILE"

        if $VALGRIND_CHECK ; then
            valgrind \
                --error-exitcode=-1 \
                --leak-check=full \
                --show-leak-kinds=all \
                --track-origins=yes \
                $NIGHTMAN_PATH -l -o "$FILE"
        else
            $NIGHTMAN_PATH -l -o "$FILE"
        fi
    done
}

check_egold () {
    cd ${FF_TEST_DIR_EGOLD}

    check_dir
}

check_sgold () {
    cd ${FF_TEST_DIR_SGOLD}

    check_dir
}

check_sgold2 () {
    cd ${FF_TEST_DIR_SGOLD2}

    check_dir
}

check_sgold2_elka () {
    cd ${FF_TEST_DIR_SGOLD2_ELKA}

    check_dir
}

case $PLATFORM in
    egold)
        check_egold
        ;;

    sgold)
        check_sgold
        ;;

    sgold2)
        check_sgold2
        ;;

    sgold2_elka)
        check_sgold2_elka
        ;;

    all)
        check_egold
        check_sgold
        check_sgold2
        check_sgold2_elka
        ;;

    *)
        echo "Aavailable platforms (lowercase available):"
        echo "[ EGOLD, SGOLD, SGOLD2, SGOLD2_ELKA, ALL ]"

        exit -1
        ;;
esac
