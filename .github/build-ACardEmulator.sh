#!/bin/bash

# CI script

set -ex -o xtrace

pushd ACardEmulator

sudo apt-get install openjdk-8-jdk coreutils

export JAVA_HOME=/usr/lib/jvm/java-8-openjdk-amd64

wget https://dl.google.com/android/repository/commandlinetools-linux-6200805_latest.zip
mkdir -p $HOME/Android/Sdk
unzip commandlinetools-linux-6200805_latest.zip -d $HOME/Android/Sdk

export ANDROID_HOME=$HOME/Android/Sdk
# Make sure emulator path comes before tools. Had trouble on Ubuntu with emulator from /tools being loaded
# instead of the one from /emulator
export PATH="$ANDROID_HOME/emulator:$ANDROID_HOME/tools:$ANDROID_HOME/tools/bin:$ANDROID_HOME/platform-tools:$PATH"

sdkmanager --sdk_root=${ANDROID_HOME} "tools"

sdkmanager --sdk_root=${ANDROID_HOME} --update
sdkmanager --sdk_root=${ANDROID_HOME} --list
sdkmanager --sdk_root=${ANDROID_HOME} "build-tools;23.0.3" "platform-tools" "platforms;android-23" "tools" "ndk;22.0.7026061"
yes | sdkmanager --sdk_root=${ANDROID_HOME} --licenses

./gradlew assemble

popd
