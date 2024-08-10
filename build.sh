#!/bin/sh

CFLAGS="-Wall -Wextra -pedantic"

if [ $1 = "static" ]; then
    LINKER_FLAGS=" -lm `pkg-config x11 --libs --static | sed -E 's/-l(\w*)/-l:lib\1.a/g'`"

elif [ $1 = "dynamic" ]; then
    LINKER_FLAGS="-lX11 -lm"

else
    echo "USAGE: build.sh <static|dynamic> <main|debug>"
    exit 1

fi

if [ $2 = "main" ]; then
    set -xe
    gcc $CFLAGS -O3 -g src/minicel.c -o main $LINKER_FLAGS

elif [ $2 = "debug" ]; then
    set -xe
    gcc $CFLAGS -DDEBUG -g src/minicel.c -o debug $LINKER_FLAGS

else
    echo "USAGE: build.sh <static|dynamic> <main|debug>"
    exit 1

fi
