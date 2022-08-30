#!/bin/bash

# Generates the bin folder and all the makefiles within it.
# Will automatically clear the bin folder before starting.
# Any arguments passed to this script will be passed to CMake.
# You shouldnt need to run this *that* often, but CMake is weird.

# Where are we?
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cd $SCRIPT_DIR

# Make sure the submodules are up to date.
git submodule init
git submodule update

# Tell CMake where the SDK is
export PICO_SDK_PATH=../lib/pico-sdk

# Clear bin folder and create a new one
rm -rI bin # I dont trust my bash skills, so you're getting prompted.
mkdir bin  # Cope.

# Make the makefiles
cd bin
cmake "$@" ../src
