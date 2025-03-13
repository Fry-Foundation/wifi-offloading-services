#!/bin/bash

start_time=$(date +%s)

# Adjust these variables to your needs; defaults below are for the Comfast CF-E375AC (Genesis-like)
RELEASE="23.05.4"
TARGET="ath79"
SUBTARGET="generic"

CURRENT_DIR=$(pwd)

TEMP_FEEDS_DIR="$CURRENT_DIR/feed"
TEMP_FEEDS_NAME="wayru_custom"

BUILD_DIR="$CURRENT_DIR/build"
FILE_TIMESTAMP=$(date +"%Y-%m-%d-%H%M%S")
VERSIONED_DIR="$BUILD_DIR/$FILE_TIMESTAMP"

SDK_DIR="$CURRENT_DIR/sdk"

# Clean up and set up directories
echo "Cleaning and setting up directories"
echo "---------------------------------------"
rm -rf "$TEMP_FEEDS_DIR"
mkdir -p "$BUILD_DIR"
mkdir -p "$VERSIONED_DIR"
mkdir -p "$TEMP_FEEDS_DIR/admin/wayru-os-services"

# Set up build source in temp feed dir
echo "Copying build source to temp feed directory"
echo "---------------------------------------"
cp -r "$CURRENT_DIR/Makefile" "$TEMP_FEEDS_DIR/admin/wayru-os-services/"
cp -r "$CURRENT_DIR/VERSION" "$TEMP_FEEDS_DIR/admin/wayru-os-services/"
cp -r "$CURRENT_DIR/source" "$TEMP_FEEDS_DIR/admin/wayru-os-services/"

# Download OpenWRT SDK (if not already downloaded)
if [ ! -d "$SDK_DIR" ]; then
    echo "Downloading OpenWrt SDK ..."
    echo "---------------------------------------"
    curl -o sdk.tar.xz "https://archive.openwrt.org/releases/$RELEASE/targets/$TARGET/$SUBTARGET/openwrt-sdk-$RELEASE-$TARGET-${SUBTARGET}_gcc-12.3.0_musl.Linux-x86_64.tar.xz"
    tar -xf sdk.tar.xz
    mv "openwrt-sdk-$RELEASE-$TARGET-${SUBTARGET}_gcc-12.3.0_musl.Linux-x86_64" $SDK_DIR
    rm sdk.tar.xz
else
    echo "OpenWrt SDK already downloaded"
    echo "---------------------------------------"
fi

# Configure feeds
echo "Configuring SDK feeds"
echo "---------------------------------------"
cd $SDK_DIR
echo "src-link $TEMP_FEEDS_NAME $TEMP_FEEDS_DIR" > feeds.conf
cat feeds.conf.default >> feeds.conf

# Remove some of the default feeds
sed -i '/luci/d' feeds.conf
sed -i '/telephony/d' feeds.conf
sed -i '/routing/d' feeds.conf

# Update feeds
echo "Updating feeds ..."
echo "---------------------------------------"
./scripts/feeds update -a

# Prepare the wayru-os-services package
echo "Preparing wayru-os-services package"
echo "---------------------------------------"
./scripts/feeds install wayru-os-services

# Configure SDK
echo "Configuring SDK"
echo "---------------------------------------"

# Deselect default settings that build all packages
rm .config
touch .config
echo "# CONFIG_ALL_NONSHARED is not set" >> .config
echo "# CONFIG_ALL_KMODS is not set" >> .config
echo "# CONFIG_ALL is not set" >> .config
echo "CONFIG_PACKAGE_wayru-os-services=y" >> .config
make defconfig

# Build
make package/wayru-os-services/download || exit 1
make package/wayru-os-services/prepare || exit 1
make package/wayru-os-services/compile || exit 1
# make -j1 V=sc package/wayru-os-services/compile || exit 1

# Finding compile target
CONFIG_LINE=$(grep 'CONFIG_TARGET_ARCH_PACKAGES=' '.config')

if [ -z "$CONFIG_LINE" ]; then
	echo "Error: Compile target not found in the configuration file; cannot determine the output directory"
    echo "Please check the output manually to find the compiled package"
	exit 1
fi

COMPILE_TARGET=$(echo $CONFIG_LINE | sed -E 's/CONFIG_TARGET_ARCH_PACKAGES="([^"]+)"/\1/')

# Move the compiled package to the build directory
echo "Moving compiled package"
echo "---------------------------------------"
mv "bin/packages/$COMPILE_TARGET/$TEMP_FEEDS_NAME"/* "$VERSIONED_DIR"

# Clean up
echo "Cleaning up"
echo "---------------------------------------"
make package/wayru-os-services/clean

end_time=$(date +%s) # Seconds since epoch
elapsed_time=$((end_time - start_time))

echo "Done! Took $elapsed_time seconds"
echo "---------------------------------------"
echo "name: wayru-os-services"
echo "arch: $COMPILE_TARGET"
echo "---------------------------------------"

exit 0
