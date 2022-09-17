#!/bin/bash

# Generates the bin folder and all the makefiles within it.
# Will automatically clear the bin folder before starting.
# Any arguments passed to this script will be passed to CMake.
# You shouldnt need to run this *that* often, but CMake is weird.

# Where is the scritp?
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cd $SCRIPT_DIR

# Make sure the submodules are up to date.
git submodule init
git submodule update

# Tell CMake where the SDK is
# export PICO_SDK_PATH=$SCRIPT_DIR/lib/pico-sdk

# Tell CMake what we're building for
# export PICO_BOARD_HEADER_DIRS=$SCRIPT_DIR

mkdir -p bin

# Make the makefiles
cd bin
cmake -DPICO_BOARD=bob -DPICO_BOARD_HEADER_DIRS=$SCRIPT_DIR -DPICO_SDK_PATH=$SCRIPT_DIR"/lib/pico-sdk" "$@" ../src
