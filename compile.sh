#!/bin/bash

set -e

START_TIME=$(date +"%Y-%m-%d-%H%M%S")
CURRENT_DIR=$(pwd)
TEMP_FEEDS_DIR="$CURRENT_DIR/feed"
TEMP_FEEDS_NAME="wayru_custom"
BUILD_DIR="$CURRENT_DIR/build"
VERSIONED_DIR="$BUILD_DIR/$START_TIME"
OPENWRT_DIR="../wayru-os"

# Clean up previous temp dirs
rm -rf "$TEMP_FEEDS_DIR"

mkdir -p "$BUILD_DIR"
mkdir -p "$VERSIONED_DIR"
mkdir -p "$TEMP_FEEDS_DIR/admin/wayru-os-services"

# Check if the build system repo exists
echo -e "\nChecking required repositories... 🕵️"
echo "---------------------------------------"
if [ ! -d "$OPENWRT_DIR/.git" ]; then
	echo "Error: The wayru-os repo does not exist"
	echo "Make sure to read the README.md file"
	exit 1
fi

# Copy relevant files to the temp feed directory
cp -r "$CURRENT_DIR/Makefile" "$TEMP_FEEDS_DIR/admin/wayru-os-services/"
cp -r "$CURRENT_DIR/VERSION" "$TEMP_FEEDS_DIR/admin/wayru-os-services/"
cp -r "$CURRENT_DIR/source" "$TEMP_FEEDS_DIR/admin/wayru-os-services/"
cp -r "$CURRENT_DIR/certificates" "$TEMP_FEEDS_DIR/admin/wayru-os-services/"
cp $CURRENT_DIR/.env "$TEMP_FEEDS_DIR/admin/wayru-os-services/"

# Move to the build system repo and get the current branch
cd "$OPENWRT_DIR"
CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
echo "Current branch is '$CURRENT_BRANCH'"

# Read the compile target
CONFIG_LINE=$(grep 'CONFIG_TARGET_ARCH_PACKAGES=' '.config')

if [ -z "$CONFIG_LINE" ]; then
	echo "Error: Compile target not found in the configuration file"
	echo "Please make sure the wayru-os repo is set up correctly"
	exit 1
fi

COMPILE_TARGET=$(echo $CONFIG_LINE | sed -E 's/CONFIG_TARGET_ARCH_PACKAGES="([^"]+)"/\1/')
echo "Compile target is '$COMPILE_TARGET'"

# Build system clean up
echo -e "\nSetting up the build environment... 🛠️"
echo "---------------------------------------"
make clean
# rm -rf feeds

# Configure custom feed
echo -e "\nUpdating and installing feeds... 🔄"
echo "---------------------------------------"
echo "src-link $TEMP_FEEDS_NAME $TEMP_FEEDS_DIR" > feeds.conf
cat feeds.conf.default >> feeds.conf
sed -i '/luci/d' feeds.conf
sed -i '/telephony/d' feeds.conf
sed -i '/routing/d' feeds.conf
sed -i '/wayru_os_feed/d' feeds.conf
./scripts/feeds update -a
./scripts/feeds install -a

# Make sure our package is selected for compilation
echo "CONFIG_PACKAGE_wayru-os-services=y" >> .config
make defconfig

# Compile
echo -e "\nBuilding... 🏗️"
echo "---------------------------------------"
make -j"$(nproc)" package/wayru-os-services/compile || exit 1
# make -j1 V=sc package/wayru-os-services/compile || exit 1

# Move the compiled package to the build directory
echo -e "\nMoving compiled package... 📦"
echo "---------------------------------------"
mv "bin/packages/$COMPILE_TARGET/$TEMP_FEEDS_NAME"/* "$VERSIONED_DIR"

echo -e "\nDone! ✨"
exit 0