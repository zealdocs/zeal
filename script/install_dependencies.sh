#!/bin/bash

REQUIRED_DEPENDENCIES=(
    git
    cmake
    make
    )

REQUIRED_DEPENDENCIES_LIBS=(
    qtbase5-dev
    extra-cmake-modules
    libqt5webchannel5-dev
    libqt5webenginewidgets5
    qtwebengine5-dev
    libarchive-dev
    libqt5x11extras5-dev
    libxcb-keysyms1-dev
    libsqlite3-dev
    )

# Function to install dependencies for Debian/Ubuntu
install_deps_debian() {
    for dep in "${REQUIRED_DEPENDENCIES_LIBS[@]}"; do
        if ! dpkg -l | grep $dep &> /dev/null;  then
            echo "$dep is not installed. Installing..."
            sudo apt-get update && sudo apt-get install -y "$dep"
        fi
    done

    for dep in "${REQUIRED_DEPENDENCIES[@]}"; do
        if ! command -v "$dep" &> /dev/null; then
            echo "$dep is not installed. Installing..."
            sudo apt-get update && sudo apt-get install -y "$dep"
        fi
    done
}


# Function to install dependencies for RHEL/Rocky Linux
install_deps_rhel() {
    
    for dep in "${REQUIRED_DEPENDENCIES[@]}"; do
        if ! command -v "$dep" &> /dev/null; then
            echo "$dep is not installed. Installing..."
            sudo yum install -y "$dep"
        fi
    done
}


# Determine the OS and install dependencies
if [ -f /etc/os-release ]; then
    . /etc/os-release
    case "$ID" in
        ubuntu|debian|raspbian|linuxmint|kali)
            install_deps_debian
            BIN_PATH="/usr/local/bin"
            CONFIG_PATH="/usr/local/etc"
            ;;
        rhel|rocky|centos)
            install_deps_rhel
            BIN_PATH="/usr/local/bin"
            CONFIG_PATH="/usr/local/etc"
            ;;
        *)
            echo "Unsupported distribution: $ID"
            exit 1
            ;;
    esac
else
    echo "Cannot determine the OS. /etc/os-release not found."
    exit 1
fi
