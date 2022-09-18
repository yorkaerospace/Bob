#!/bin/bash

# Generates the bin folder and all the makefiles within it.
# Will automatically clear the bin folder before starting.
# Set -D to generate a debug build, which will be copied directly to RAM.

# Where are we?
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cd $SCRIPT_DIR

# Make sure the submodules are up to date.
git submodule init
git submodule update

# Tell CMake where the SDK is
export PICO_SDK_PATH=$SCRIPT_DIR/lib/pico-sdk

# Tell CMake what we're building for
export PICO_BOARD_HEADER_DIRS=$SCRIPT_DIR

if [ ! -d bin ]
   then
       mkdir bin
fi

# Make the makefiles
cd bin
cmake ../src
