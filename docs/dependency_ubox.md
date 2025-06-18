# ubox
ubox is a set of C utilities for OpenWrt.

This guide describes how to clone the code, build and install the ubox utilities.

These instructions are for an Arch Linux system, but they can be adapted for other Linux distributions as well.

## Setup

### Clone, build, install
```bash
git clone https://git.openwrt.org/project/ubox.git

cd ubox

mkdir build
cd build

cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
make
sudo make install

# add `/usr/local/lib` to ld.so.conf.d if not already present
echo "/usr/local/lib" > /etc/ld.so.conf.d/ubox.conf
sudo ldconfig
```

The ubox utilities will be installed to `/usr/local/bin` and libraries to `/usr/local/lib`.