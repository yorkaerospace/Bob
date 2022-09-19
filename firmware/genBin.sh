#!/bin/bash

# Generates the bin folder and all the makefiles within it.
# Will automatically clear the bin folder before starting.
# Any arguments passed to this script will be passed to CMake

# Where is the script?
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cd $SCRIPT_DIR

# Make sure the submodules are up to date.
git submodule init
git submodule update

CMAKE_ARGS="-DPICO_BOARD=bob -DPICO_BOARD_HEADER_DIRS=$SCRIPT_DIR/src $@"

mkdir -p bin

# Make the makefiles
cd bin
cmake $CMAKE_ARGS ..
