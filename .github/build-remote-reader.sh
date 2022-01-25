#!/bin/bash

# CI script

set -ex -o xtrace

pushd remote-reader

./gradlew build

popd
