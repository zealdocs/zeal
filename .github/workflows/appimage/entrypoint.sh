#!/bin/bash

# Should be in .gitignore.
export APPIMAGE_BUILD_DIR=build.appimage

# Install dependencies
if [ ! -z ${INPUT_APT_DEPENDENCIES+x} ]; then
    apt-get update -q -y
    apt-get install -q -y --no-install-recommends ${INPUT_APT_DEPENDENCIES}
fi

# Run appimage-builder
appimage-builder --skip-test --build-dir ${APPIMAGE_BUILD_DIR} --appdir ${APPIMAGE_BUILD_DIR}/AppDir --recipe ${INPUT_RECIPE}
