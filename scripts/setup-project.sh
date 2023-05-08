#!/bin/bash

set -e

pushd "$(dirname ${0})/.."

sdk_zip=DA16200_DA16600_SDK_FreeRTOS_v3.2.7.1.zip

rm -rf da16k-sdk-zip
mkdir da16k-sdk-zip
pushd da16k-sdk-zip >/dev/null
  echo Extracting ${sdk_zip}...
  unzip -q ../${sdk_zip}
popd >/dev/null

echo Overlaying da16k-sdk directory with zip contents...
cp -rn da16k-sdk-zip/* ./
cp -rn da16k-sdk-zip/.project ./
rm -rf da16k-sdk-zip

pushd tools/util >/dev/null
sh ./set_linux_perm.sh
popd >/dev/null

echo Done.



