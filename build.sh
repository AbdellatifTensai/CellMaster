#!/bin/sh

set -xe

CFLAGS="-Wall -Wextra -pedantic"
LINKER_FLAGS="-lX11"

mkdir -p build

gcc $CFLAGS -O3 -g src/main.c -o build/main $LINKER_FLAGS
gcc $CFLAGS -DDEBUG -g src/main.c -o build/main-debug $LINKER_FLAGS
