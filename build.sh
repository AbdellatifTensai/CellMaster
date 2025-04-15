#!/bin/sh

set -xe

CFLAGS="-Wall -Wextra -pedantic"

# static linking
#LINKER_FLAGS=" -lm `pkg-config x11 --libs --static | sed -E 's/-l(\w*)/-l:lib\1.a/g'`"
    
# dynamic linking
LINKER_FLAGS="-lX11"

mkdir -p build

gcc $CFLAGS -O3 -g src/main.c -o build/main $LINKER_FLAGS
gcc $CFLAGS -DDEBUG -g src/main.c -o build/main-debug $LINKER_FLAGS
