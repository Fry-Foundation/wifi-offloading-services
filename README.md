# wayru-os-services
Set of services for OpenWrt-based firmware.

**wayru-agent**
- registration
- access token
- mqtt
- metrics
- updates
- misc

**wayru-collector**
- log collection

**wayru-config**
- config sync

## Dependencies
### Build
Install the OpenWrt build system dependencies needed for your system: [Build system setup](https://openwrt.org/docs/guide-developer/toolchain/install-buildsystem)

Also:
- cmake
- just
- go
- docker

### Libraries
- libcurl
- libjson-c
- libmosquitto
- libssl
- liblua
- libubox, see [docs/dependency_ubox](./docs/dependency_ubox.md)
- libubus, see [docs/dependency_ubus](./docs/dependency_ubus.md)

## Development
This project is written in C with supporting shell and Lua scripts.

It is split between apps and libraries. Check the `./apps` and `./libs` directories.

## Workflows
Build the project; will build all apps:
```bash
just build
```

Clean the project:
```bash
just clean
```

Run any of the apps in the `./apps` directory:
```bash
just run <app>
```

Compile the project:
```bash
just compile arch=<value>
```
> Compilations are done using the OpenWrt SDK inside a Debian Docker container. The operation is orchestrated by a small program built with Go.

The compiled package will be available in a new folder called `./output/<arch>/<package>.ipk`

You can then transfer the compiled package to a router for testing.

Format the C files in the project:
```bash
just format
```
> Requires `clang-format` to be installed.

Build compilation symbols:
```bash
just compdb
```
This can help if your IDE is not recognizing the include paths correctly and is reporting a lot of linting errors.
