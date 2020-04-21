#!/bin/bash

# Install dependencies
if [ ! -z ${INPUT_APT_DEPENDENCIES+x} ]; then
    apt-get update -q -y
    apt-get install -q -y --no-install-recommends ${INPUT_APT_DEPENDENCIES}
fi

# Run appimage-builder
appimage-builder --skip-test --recipe ${INPUT_RECIPE}
