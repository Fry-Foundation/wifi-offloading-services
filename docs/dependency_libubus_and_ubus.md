# libubus and ubus
> Make sure you install the `libubox` utilities first.

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
echo "/usr/local/lib" | sudo tee -a /etc/ld.so.conf.d/ubus.conf
sudo ldconfig
```

### Create a ubus user and group
```bash
sudo useradd --system --no-create-home --shell /usr/bin/nologin ubus
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
Type=simple
User=ubus
Group=ubus
RuntimeDirectory=ubus
RuntimeDirectoryMode=0775
UMask=0002
ExecStart=/usr/local/sbin/ubusd -s /run/ubus/ubus.sock

[Install]
WantedBy=multi-user.target
```

### Enable and start
```
sudo systemctl enable ubus
sudo systemctl start ubus
```

### Make sure your user has permissions
```
sudo usermod -aG ubus $USER
newgrp ubus
```

### Make sure your user is part of the ubus ACL
```
sudo mkdir -p /usr/share/acl.d
sudo tee /usr/share/acl.d/$USER.json << 'EOF'
{
    "user": "$USER",
    "access": {
        "*": {
            "methods": ["*"]
        }
    },
    "send": ["*"],
    "subscribe": ["*"],
    "publish": ["*"]
}
EOF

sudo systemctl restart ubus
```

This ACL configuration allows the user `$USER` to:
- Access all objects (`"*"`) and call all methods (`"methods": ["*"]`)
- Send events to all objects (`"send": ["*"]`)
- Subscribe to events from all objects (`"subscribe": ["*"]`)
- Publish events to all objects (`"publish": ["*"]`)

For more restrictive access, you can specify individual object names and methods instead of using wildcards.
