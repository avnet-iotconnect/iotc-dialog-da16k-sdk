#!/bin/bash

echo "Avnet IoTConnect Dialog DA16K AT SDK"

if [ "$#" -ne 1 ]; then
    echo "Source path not given!"
    echo "Usage: $0 <da16k_sdk_path>"
    echo "Where <da16k_sdk_path> is the path to a DA16K FreeRTOS SDK. Example:"
    echo "$0 ~/DA16200_DA16600_SDK_FreeRTOS_v3.2.8.1"
    exit 1
fi

source_path="$1"

# Check if given path exists
if [ ! -d "$source_path" ]; then
    echo "Invalid source path! Does it exist?"
    exit 1
fi

if [ ! -f "$source_path/utility/cfg_generator/da16x_gencfg" ] || [ ! -f "$source_path/tools/util/set_linux_perm.sh" ] || [ ! -d "$source_path/apps" ]; then
    echo "Error: The specified path does not seem to contain a DA16K FreeRTOS SDK."
    exit 1
fi

# Make sure submodules are pulled and up to date.
echo "Updating submodules..."
git submodule update --init --recursive

# Copy files from the given path to the script's path whilst *not* overwriting the files in this repo.
echo "Copying SDK files..."
cp -n -r "$source_path"/* .

# Setup permissions for the SDK's script files
echo "Setting up SDK script permissions..."
pushd "$source_path"/tools/util
chmod +x set_linux_perm.sh
./set_linux_perm.sh
popd

echo "Done."