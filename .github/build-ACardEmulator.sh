#!/bin/bash

# CI script

set -ex -o xtrace

pushd ACardEmulator

git submodule update --init --recursive .

./gradlew assemble

popd
