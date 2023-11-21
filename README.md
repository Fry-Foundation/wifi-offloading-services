# wayru-os-services
Wayru services on OpenWrt:
- Identity
- Onboarding
- API

## Dependencies
> Work in progress. Dependencies should be version controlled for testing and production builds.

### Install the `libmicrohttpd` dependency

**Debian-based linux**

Install this package with `apt-get`:

```bash
sudo apt-get update
sudo apt-get install libmicrohttpd-dev
```

**macOS**

Install this package with `brew`:

```bash
berw update
brew install libmicrohttpd
```

**Windows**

Not tested yet, but there are Windows binaries available: https://www.gnu.org/software/libmicrohttpd/

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

### Requirements
- Meet the OpenWrt build system requirements on your local machine: https://openwrt.org/docs/guide-developer/toolchain/install-buildsystem
- Relevant wayru-os repositories cloned in your local machine:
    - [wayru-os](https://github.com/Wayru-Network/wayru-os.git)
    - [wayru-os-feed](https://github.com/Wayru-Network/wayru-os-feed.git)
    - wayru-os-services (this repo)

### Build system setup
Install the tools and toolchain in the `wayru-os` repo.

```bash
make tools/install
make toolchain/install
```

Make sure the `wayru-os-feed` repo is added as a feed to the build system. The `feeds.conf` file in the root directory should include the following line:

```bash
src-git wayru_os_feed https://github.com/Wayru-Network/wayru-os-feed.git
```

Update the feed.

```bash
./scripts/feeds update wayru_os_feed
```

Install the feed's packages.

```bash
./scripts/feeds install -a -p wayru_os_feed
```

### Build the package
Build the package with the OpenWrt build system.

```bash
make package/wayru-os-services/compile
```
