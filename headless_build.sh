#!/bin/bash

E2STUDIO_DIR=~/.local/share/renesas/e2_studio/eclipse
E2STUDIO_INI=$E2STUDIO_DIR/e2studio.ini
BASE_DIR=$PWD
WORKSPACE_DIR=$BASE_DIR/.tmp_workspace
PROJECT_DIR_16600=$BASE_DIR/apps/common/examples/Network/IoTConnect_Client/projects/da16600
PROJECT_DIR_16200=$BASE_DIR/apps/common/examples/Network/IoTConnect_Client/projects/da16200


if [ ! -f "$E2STUDIO_INI" ]; then
    echo "e2studio.ini not found at expected location ($E2STUDIO_INI) - is e2 studio installed?"
    exit 1
fi

cp "$E2STUDIO_INI" "$E2STUDIO_DIR/eclipse.ini"

pushd utility/cfg_generator/

    FLASHTYPESDIR=../../tools/SBOOT/SFDP
    CURCFG="MX25U3235F"

    echo "Generating build config: $CURCFG"
    python3 da16x_gencfg.py $CURCFG 4M 4M

    pushd "$E2STUDIO_DIR"

    rm -rf "$WORKSPACE_DIR"
    mkdir -p "$WORKSPACE_DIR"
    
    ./e2studio -nosplash --launcher.suppressErrors -application org.eclipse.cdt.managedbuilder.core.headlessbuild -data "$WORKSPACE_DIR" -import "$PROJECT_DIR_16200" -build all
    
    rm -rf "$WORKSPACE_DIR"
    mkdir -p "$WORKSPACE_DIR"
    ./e2studio -nosplash --launcher.suppressErrors -application org.eclipse.cdt.managedbuilder.core.headlessbuild -data "$WORKSPACE_DIR" -import "$PROJECT_DIR_16600" -build all

    rm -rf "$WORKSPACE_DIR"

    popd


popd

mkdir -p ./images
cp $PROJECT_DIR_16200/img/*_FRTOS* ./images
cp $PROJECT_DIR_16600/img/*_FRTOS* ./images

exit 0

for file in "$FLASHTYPESDIR"/*.bin; do
    # Extract the filename without the extension
    FILENAME=$(basename "$file")
    CURCFG="${FILENAME%.*}"
    # Print the filename without the extension
    echo "Generating build config: $CURCFG"
    python3 da16x_gencfg.py $CURCFG 4M 4M
done

popd   