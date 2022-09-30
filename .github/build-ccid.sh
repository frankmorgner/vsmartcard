#!/bin/bash

# CI script to build for "ubuntu", "coverity"

set -ex -o xtrace

# todo openpace

pushd ccid
git submodule update --init .

sudo apt-get install gengetopt help2man libpcsclite-dev

autoreconf -vis

./configure

case "$1" in
    ubuntu)
        make distcheck CLANGTIDY=""
        ;;
    coverity)
        # exclude opensc from analysis by building it here
        make -C src/OpenSC
esac

popd
