# wayru-os-services
Services for wayru-os:
- Access key
- Setup / onboarding
- Accounting

## Dependencies
> Work in progress. Dependencies should be version controlled for testing and production builds.

> OpenSSL / libssl is also needed. Check Makefile. @todo document that dependency here too.

### Install the `libcurl`, `json-c`, `mosquitto` dependencies

**Debian-based linux**

Install with `apt-get`:

```bash
sudo apt-get update
sudo apt-get install libcurl4-gnutls-dev
sudo apt-get install libjson-c-dev
sudo apt-get install libmosquitto-dev
```

## Clone the repository

```bash
git clone https://github.com/Wayru-Network/wayru-os-services.git

```

## Developing
To run the project locally you can run the `dev` script found at the root of this project:

```bash
bash dev
```

## Compiling for OpenWrt

Compilations are done using the OpenWrt build system.

The script `compile` automates the process, **but the build system needs to be set up correctly**.

### Requirements
- Meet the [OpenWrt build system requirements](https://openwrt.org/docs/guide-developer/toolchain/install-buildsystem) on your local machine or VM 
- Clone the [wayru-os](https://github.com/Wayru-Network/wayru-os.git) repository

Note that your directory structure should look like this:
```
/path/to/
├── wayru-os/
└── wayru-os-services/
```

### Build system setup
Install the tools and toolchain, and select your desired architecture in the `wayru-os` repo:

```bash
# Install tools and toolchain
make tools/install
make toolchain/install

# Select your desired architecture and target device
make menuconfig
```

### Build the package
Run the `compile` script from the `wayru-os-services` repo:

```bash
bash compile
```

The compiled package will be available in a new folder called `./build/{date}`

You can then transfer the compiled package to a router for testing.

## Debugging in OpenWrt

### Building with debug symbols
Use the commented out `Build/Compile` step in the main Makefile that includes the `-g -O0` flags to build with debug information and remove compiler optimizations.

### Configuring the build system
Configure the build system to include debug information in the `wayru-os` repo with `make menuconfig`:

Open the **Global build settings** menu, and make sure these are checked:
- Collect kernel debug information
- Compile packages with debugging info

Make sure that **Binary stripping method** is set to `none`

Make sure these are not checked:
- Make debug information reproducible
- Strip unnecessary exports from the kernel image
- Strip unnecessary functions from libraries

Also, within the **Kernel built options** submenu, make sure that these are checked:
- Compile the kernel with debug filesystem enabled
- Compile the kernel with symbol table information
- Compile the kernel with debug information 

### Installing the debugger
On your OpenWrt device, install `gdb`:
```bash
opkg update
opkg install gdb
```

### Running the debugger
On OpenWrt:
- Run `gdb` with the binary from its install location `/usr/bin`
- Set the arguments needed by the binary for proper operation
- Set breakpoints at certain line numbers or function names
- Run the binary

```bash
gdb /usr/bin/wayru-os-services

# Within the gdb process
(gdb) set args --config-enabled "1" --config-main-api "https://api.internal.wayru.tech" --config-accounting-enabled "1" --config-accounting-interval "120" --config-accounting-api "https://wifi.api.internal.wayru.tech" --config-access-interval "120" --config-device-status-interval "120" --config-setup-interval "300" --config-console-log-level "4"

(gdb) break main.c:20

(gdb) run
```

## Tooling

### Formatting
Install the clang-format util:
```bash
sudo apt install clang-format
```


Run the `format-c` script from the `wayru-os-services` repo:

```bash
bash format-c
```

### LSP
The langauge server protocol (LSP) for C should work out of the box with VSCode.

On other editors you can use clangd. But make sure to run `generate_compile_commands.sh` so that clangd correctly recognizes the include paths.

You will need to have the `bear` tool installed to run this script: [https://github.com/rizsotto/Bear](https://github.com/rizsotto/Bear)
