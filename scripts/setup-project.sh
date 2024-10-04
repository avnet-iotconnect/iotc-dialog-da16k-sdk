#!/bin/bash

set -e

scripts_dir=$(dirname "${0}")
repo_root="${scripts_dir}/../"
cd "${repo_root}"

da_sdk_path="./DA16K_SDK_FreeRTOS"
da_sdk_zip="DA16200_DA16600_SDK_FreeRTOS_v3.2.8.1.zip"


# Check if given path exists
if [ ! -f "${da_sdk_zip}" ]; then
    echo "${da_sdk_zip} appears to be missing."
    echo "You must download it and place it into the root of this repo."
    exit 1
fi

echo "Preparing the Avnet IoTConnect Dialog DA16K SDK..."
# Make sure submodules are pulled and up to date.
echo "Updating submodules..."
git submodule update --init --recursive

rm -rf "${da_sdk_path}"
mkdir -p "${da_sdk_path}"
pushd "${da_sdk_path}" >/dev/null
  echo "Extracting the ${da_sdk_zip}..."
  unzip -q "../${da_sdk_zip}"
popd >/dev/null

if [ ! -f "${da_sdk_path}/utility/cfg_generator/da16x_gencfg" ] || [ ! -f "${da_sdk_path}/tools/util/set_linux_perm.sh" ] || [ ! -d "${da_sdk_path}/apps" ]; then
    echo "Error: The SDK does not seem to contain the neccessary files."
    exit 1
fi

# Copy files from the given path to the script's path whilst *not* overwriting the files in this repo.
echo "Copying SDK files..."
cp -r --update=none "${da_sdk_path}"/* .

# Setup permissions for the SDK's script files
echo "Setting up SDK script permissions..."
pushd "./tools/util" >/dev/null
  bash ./set_linux_perm.sh
popd >/dev/null

echo "Cleaning up..."
rm -rf "${da_sdk_path}"

echo "Done."