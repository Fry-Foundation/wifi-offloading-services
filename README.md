# wayru-os-services
Services for wayru-os:
- Access key
- Setup / onboarding
- Accounting

## Dependencies
> Work in progress. Dependencies should be version controlled for testing and production builds.

### Install the `libcurl` and `json-c` dependencies

**Debian-based linux**

Install with `apt-get`:

```bash
sudo apt-get update
sudo apt-get install libcurl4-gnutls-dev
sudo apt-get install libjson-c-dev
```

**macOS**

Install with `brew`:

```bash
berw update
brew install curl
brew install json-c
```

**Windows**

Not tested yet.

## Clone the repository

```bash
git clone https://github.com/Wayru-Network/wayru-os-services.git

```

## Running
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
Install the tools and toolchain in the `wayru-os` repo:

```bash
make tools/install
make toolchain/install
```

### Build the package
Run the `compile` script from the `wayru-os-services` repo:

```bash
bash compile
```

The compiled package will be available in a new folder called `./build/{date}`