#!/bin/bash

# CI script to build for "ubuntu", "coverity", "macos"

set -ex -o xtrace

pushd virtualsmartcard

DEPS="help2man automake libtool"

case "$1" in
    ubuntu|coverity)
        DEPS="$DEPS libpcsclite-dev libqrencode-dev"
        ;;
    macos)
        DEPS="$DEPS gengetopt"
        ;;
esac

case "$1" in
    ubuntu|coverity)
        sudo apt-get install -y $DEPS
        ;;
    macos)
        brew install $DEPS
        ;;
esac

autoreconf -vis

./configure

case "$1" in
    ubuntu)
        make distcheck
        ;;
    macos)
        make osx
        ;;
esac

popd
