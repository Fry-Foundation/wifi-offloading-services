#!/bin/bash

set -e

START_TIME=$(date +"%Y-%m-%d-%H%M%S")
CURRENT_DIR=$(pwd)
TEMP_FEEDS_DIR="$CURRENT_DIR/feed"
TEMP_FEEDS_NAME="wayru_custom"
BUILD_DIR="$CURRENT_DIR/build"
VERSIONED_DIR="$BUILD_DIR/$START_TIME"
OPENWRT_DIR="$CURRENT_DIR/openwrt"

# Clean up previous temp dirs
rm -rf "$TEMP_FEEDS_DIR"

mkdir -p "$BUILD_DIR"
mkdir -p "$VERSIONED_DIR"
mkdir -p "$TEMP_FEEDS_DIR/admin/wayru-os-services"

# Clone OpenWrt repository in a temporary location
# Check if the OpenWrt directory already exists, and if it does, skip the clone step
if [ -d "$OPENWRT_DIR" ]; then
    echo -e "\nOpenWrt repository already exists ... skipping clone step"
else
    echo -e "\nCloning OpenWrt repository"
    echo "---------------------------------------"
    git clone https://git.openwrt.org/openwrt/openwrt.git "$OPENWRT_DIR"
fi

cd "$OPENWRT_DIR"
git checkout v23.05.4  # Cambiar a la rama/tag deseada
CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
echo "Current branch is '$CURRENT_BRANCH'"

# Copy relevant files to the temp feed directory
cp -r "$CURRENT_DIR/Makefile" "$TEMP_FEEDS_DIR/admin/wayru-os-services/"
cp -r "$CURRENT_DIR/VERSION" "$TEMP_FEEDS_DIR/admin/wayru-os-services/"
cp -r "$CURRENT_DIR/source" "$TEMP_FEEDS_DIR/admin/wayru-os-services/"

# Set up target in .config
echo -e "\nConfiguring build target... ⚙️"
echo "---------------------------------------"

cat <<EOT >> .config
CONFIG_TARGET_ath79=y
CONFIG_TARGET_ath79_generic=y
CONFIG_TARGET_ath79_generic_DEVICE_comfast_cf-e375ac=y
CONFIG_PACKAGE_wayru-os-services=y
EOT

echo -e "\nApplying configuration with make defconfig... 🔧"
make defconfig

# Read the compile target
CONFIG_LINE=$(grep 'CONFIG_TARGET_ARCH_PACKAGES=' '.config')

if [ -z "$CONFIG_LINE" ]; then
	echo "Error: Compile target not found in the configuration file"
	echo "Please make sure the wayru-os repo/openwrt is set up correctly"
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
make toolchain/install
make tools/install
# make -j"$(nproc)" package/wayru-os-services/compile || exit 1
make -j1 V=sc package/wayru-os-services/compile || exit 1

# Move the compiled package to the build directory
echo -e "\nMoving compiled package... 📦"
echo "---------------------------------------"
mv "bin/packages/$COMPILE_TARGET/$TEMP_FEEDS_NAME"/* "$VERSIONED_DIR"

echo -e "\nDone! ✨"
exit 0
