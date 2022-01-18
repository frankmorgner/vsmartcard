#!/bin/bash

# CI script to build for "ubuntu", "coverity", "mingw-32", "mingw-64", "macos"

set -ex -o xtrace

pushd pcsc-relay

DEPS="gengetopt help2man automake"

case "$1" in
    ubuntu|coverity)
        DEPS="$DEPS libnfc-dev libpcsclite-dev"
        ;;
    mingw-32)
        DEPS="$DEPS binutils-mingw-w64-i686 gcc-mingw-w64-i686 mingw-w64-i686-dev"
        ;;
    mingw-64)
        DEPS="$DEPS binutils-mingw-w64-x86-64 gcc-mingw-w64-x86-64 mingw-w64-x86-64-dev"
        ;;
    macos)
        DEPS="$DEPS libnfc"
        ;;
esac

case "$1" in
    ubuntu|coverity|mingw-32|mingw-64)
        sudo apt-get install -y $DEPS
        ;;
    macos)
        brew install $DEPS
        ;;
esac

autoreconf -vis

case "$1" in
    ubuntu|coverity|macos)
        ./configure
        ;;
    mingw-32)
        env ac_cv_func_malloc_0_nonnull=yes ac_cv_func_realloc_0_nonnull=yes ./configure --host=i686-w64-mingw32 --target=i686-w64-mingw32 CFLAGS=-I/usr/i686-w64-mingw32/include LDFLAGS=-L/usr/i686-w64-mingw32/lib  PCSC_LIBS=-lwinscard
        ;;
    mingw-64)
        env ac_cv_func_malloc_0_nonnull=yes ac_cv_func_realloc_0_nonnull=yes ./configure --host=x86_64-w64-mingw32 --target=x86_64-w64-mingw32 CFLAGS=-I/usr/x86_64-w64-mingw32/include LDFLAGS=-L/usr/x86_64-w64-mingw32/lib PCSC_LIBS=-lwinscard
        ;;
esac

case "$1" in
    ubuntu|macos)
        make distcheck
        ;;
    mingw-32|mingw-64)
        make
        ;;
esac

popd
