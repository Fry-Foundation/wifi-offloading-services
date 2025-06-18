# ubus
> Make sure you install the `ubox` utilities first.

The OpenWrt `ubusd` and `libubus` are needed for communication between the services.

This guide describes how to clone the code, build and install the `ubusd` daemon and the `libubus` library.

These instructions are for an Arch Linux system, but they can be adapted for other Linux distributions as well.

## Setup

### Clone, build, install
```bash
git clone https://git.openwrt.org/project/ubus.git

cd ubus

mkdir build
cd build

cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
make
sudo make install

# add `/usr/local/lib` to ld.so.conf.d
echo "/usr/local/lib" > /etc/ld.so.conf.d/ubus.conf
sudo ldconfig
```

### Create a ubus user and group
```bash
sudo useradd --system --no-create-home --shell /usr/bin/nologin ubus

sudo mkdir -p /run/ubus
sudo chown ubus:ubus /run/ubus
sudo chmod 0750 /run/ubus
```

### Register ubusd with systemd
Create `/etc/systemd/system/ubus.service`.
```bash
sudo touch /etc/systemd/system/ubus.service
sudo chmod 644 /etc/systemd/system/ubus.service
```

Edit the file (`sudo nano /etc/systemd/system/ubus.service`):
```bash
[Unit]
Description=ubusd daemon
After=network.target

[Service]
ExecStart=/usr/local/sbin/ubusd --listen /run/ubus/ubus.sock
User=ubus
Group=ubus
AmbientCapabilities=

[Install]
WantedBy=multi-user.target
```
