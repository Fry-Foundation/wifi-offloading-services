include $(TOPDIR)/rules.mk

# Name, version and release number
# The name and version of your package are used to define the variable to point to the build directory of your package: $(PKG_BUILD_DIR)
PKG_NAME:=wayru-os-services
PKG_VERSION:=$(shell cat VERSION)
PKG_RELEASE:=1

# Source settings (i.e. where to find the source codes)
# This is a custom variable, used below
SOURCE_DIR:=source

include $(INCLUDE_DIR)/package.mk

# Package definition; instructs on how and where our package will appear in the overall configuration menu ('make menuconfig')
define Package/wayru-os-services
  SECTION:=admin
  CATEGORY:=Administration
  TITLE:=Wayru config daemon and scripts
  DEPENDS:=+libcurl +libjson-c +libopenssl +libmosquitto
endef

# Package description; a more verbose description on what our package does
define Package/wayru-os-services/description
  Services for access, setup, and accounting for wayru-os
endef

# Package preparation instructions; create the build directory and copy the source code. 
# The last command is necessary to ensure our preparation instructions remain compatible with the patching system.
define Build/Prepare

		mkdir -p $(PKG_BUILD_DIR)
		cp -r $(SOURCE_DIR)/* $(PKG_BUILD_DIR)
		$(Build/Patch)
endef

# Package build instructions; invoke the target-specific compiler to first compile the source file, and then to link the file into the final executable
define Build/Compile
		$(TARGET_CC) $(TARGET_CFLAGS) -I$(PKG_BUILD_DIR) -o $(PKG_BUILD_DIR)/main.o -c $(PKG_BUILD_DIR)/main.c
		$(TARGET_CC) $(TARGET_CFLAGS) -I$(PKG_BUILD_DIR) -o $(PKG_BUILD_DIR)/services/access.o -c $(PKG_BUILD_DIR)/services/access.c
		$(TARGET_CC) $(TARGET_CFLAGS) -I$(PKG_BUILD_DIR) -o $(PKG_BUILD_DIR)/services/accounting.o -c $(PKG_BUILD_DIR)/services/accounting.c
		$(TARGET_CC) $(TARGET_CFLAGS) -I$(PKG_BUILD_DIR) -o $(PKG_BUILD_DIR)/services/config.o -c $(PKG_BUILD_DIR)/services/config.c
		$(TARGET_CC) $(TARGET_CFLAGS) -I$(PKG_BUILD_DIR) -o $(PKG_BUILD_DIR)/services/device_data.o -c $(PKG_BUILD_DIR)/services/device_data.c
		$(TARGET_CC) $(TARGET_CFLAGS) -I$(PKG_BUILD_DIR) -o $(PKG_BUILD_DIR)/services/device_status.o -c $(PKG_BUILD_DIR)/services/device_status.c
		$(TARGET_CC) $(TARGET_CFLAGS) -I$(PKG_BUILD_DIR) -o $(PKG_BUILD_DIR)/services/did-key.o -c $(PKG_BUILD_DIR)/services/did-key.c
		$(TARGET_CC) $(TARGET_CFLAGS) -I$(PKG_BUILD_DIR) -o $(PKG_BUILD_DIR)/services/end_report.o -c $(PKG_BUILD_DIR)/services/end_report.c
		$(TARGET_CC) $(TARGET_CFLAGS) -I$(PKG_BUILD_DIR) -o $(PKG_BUILD_DIR)/services/setup.o -c $(PKG_BUILD_DIR)/services/setup.c
		$(TARGET_CC) $(TARGET_CFLAGS) -I$(PKG_BUILD_DIR) -o $(PKG_BUILD_DIR)/services/mqtt-cert.o -c $(PKG_BUILD_DIR)/services/mqtt-cert.c
		$(TARGET_CC) $(TARGET_CFLAGS) -I$(PKG_BUILD_DIR) -o $(PKG_BUILD_DIR)/services/mqtt.o -c $(PKG_BUILD_DIR)/services/mqtt.c
		$(TARGET_CC) $(TARGET_CFLAGS) -I$(PKG_BUILD_DIR) -o $(PKG_BUILD_DIR)/services/monitoring.o -c $(PKG_BUILD_DIR)/services/monitoring.c
		$(TARGET_CC) $(TARGET_CFLAGS) -I$(PKG_BUILD_DIR) -o $(PKG_BUILD_DIR)/services/env.o -c $(PKG_BUILD_DIR)/services/env.c
		$(TARGET_CC) $(TARGET_CFLAGS) -I$(PKG_BUILD_DIR) -o $(PKG_BUILD_DIR)/lib/console.o -c $(PKG_BUILD_DIR)/lib/console.c
		$(TARGET_CC) $(TARGET_CFLAGS) -I$(PKG_BUILD_DIR) -o $(PKG_BUILD_DIR)/lib/curl_helpers.o -c $(PKG_BUILD_DIR)/lib/curl_helpers.c
		$(TARGET_CC) $(TARGET_CFLAGS) -I$(PKG_BUILD_DIR) -o $(PKG_BUILD_DIR)/lib/key_pair.o -c $(PKG_BUILD_DIR)/lib/key_pair.c
		$(TARGET_CC) $(TARGET_CFLAGS) -I$(PKG_BUILD_DIR) -o $(PKG_BUILD_DIR)/lib/requests.o -c $(PKG_BUILD_DIR)/lib/requests.c
		$(TARGET_CC) $(TARGET_CFLAGS) -I$(PKG_BUILD_DIR) -o $(PKG_BUILD_DIR)/lib/scheduler.o -c $(PKG_BUILD_DIR)/lib/scheduler.c
		$(TARGET_CC) $(TARGET_CFLAGS) -I$(PKG_BUILD_DIR) -o $(PKG_BUILD_DIR)/lib/script_runner.o -c $(PKG_BUILD_DIR)/lib/script_runner.c

		$(TARGET_CC) $(TARGET_LDFLAGS) \
			$(PKG_BUILD_DIR)/main.o \
			$(PKG_BUILD_DIR)/services/access.o \
			$(PKG_BUILD_DIR)/services/accounting.o \
			$(PKG_BUILD_DIR)/services/config.o \
			$(PKG_BUILD_DIR)/services/device_data.o \
			$(PKG_BUILD_DIR)/services/device_status.o \
			$(PKG_BUILD_DIR)/services/did-key.o \
			$(PKG_BUILD_DIR)/services/end_report.o \
			$(PKG_BUILD_DIR)/services/setup.o \
			$(PKG_BUILD_DIR)/services/mqtt-cert.o \
			$(PKG_BUILD_DIR)/services/mqtt.o \
			$(PKG_BUILD_DIR)/services/monitoring.o \
			$(PKG_BUILD_DIR)/services/env.o \
			$(PKG_BUILD_DIR)/lib/console.o \
			$(PKG_BUILD_DIR)/lib/curl_helpers.o \
			$(PKG_BUILD_DIR)/lib/key_pair.o \
			$(PKG_BUILD_DIR)/lib/requests.o \
			$(PKG_BUILD_DIR)/lib/scheduler.o \
			$(PKG_BUILD_DIR)/lib/script_runner.o \
			-o $(PKG_BUILD_DIR)/wayru-os-services \
			-lcurl -ljson-c -lssl -lcrypto -lmosquitto
endef

# define Build/Compile
# 		$(TARGET_CC) $(TARGET_CFLAGS) -g -O0 -I$(PKG_BUILD_DIR) -o $(PKG_BUILD_DIR)/main.o -c $(PKG_BUILD_DIR)/main.c
# 		$(TARGET_CC) $(TARGET_CFLAGS) -g -O0 -I$(PKG_BUILD_DIR) -o $(PKG_BUILD_DIR)/services/access.o -c $(PKG_BUILD_DIR)/services/access.c
# 		$(TARGET_CC) $(TARGET_CFLAGS) -g -O0 -I$(PKG_BUILD_DIR) -o $(PKG_BUILD_DIR)/services/accounting.o -c $(PKG_BUILD_DIR)/services/accounting.c
# 		$(TARGET_CC) $(TARGET_CFLAGS) -g -O0 -I$(PKG_BUILD_DIR) -o $(PKG_BUILD_DIR)/services/config.o -c $(PKG_BUILD_DIR)/services/config.c
# 		$(TARGET_CC) $(TARGET_CFLAGS) -g -O0 -I$(PKG_BUILD_DIR) -o $(PKG_BUILD_DIR)/services/device_data.o -c $(PKG_BUILD_DIR)/services/device_data.c
# 		$(TARGET_CC) $(TARGET_CFLAGS) -g -O0 -I$(PKG_BUILD_DIR) -o $(PKG_BUILD_DIR)/services/device_status.o -c $(PKG_BUILD_DIR)/services/device_status.c
# 		$(TARGET_CC) $(TARGET_CFLAGS) -g -O0 -I$(PKG_BUILD_DIR) -o $(PKG_BUILD_DIR)/services/did-key.o -c $(PKG_BUILD_DIR)/services/did-key.c
# 		$(TARGET_CC) $(TARGET_CFLAGS) -g -O0 -I$(PKG_BUILD_DIR) -o $(PKG_BUILD_DIR)/services/end_report.o -c $(PKG_BUILD_DIR)/services/end_report.c
# 		$(TARGET_CC) $(TARGET_CFLAGS) -g -O0 -I$(PKG_BUILD_DIR) -o $(PKG_BUILD_DIR)/services/setup.o -c $(PKG_BUILD_DIR)/services/setup.c
# 		$(TARGET_CC) $(TARGET_CFLAGS) -g -O0 -I$(PKG_BUILD_DIR) -o $(PKG_BUILD_DIR)/lib/console.o -c $(PKG_BUILD_DIR)/lib/console.c
# 		$(TARGET_CC) $(TARGET_CFLAGS) -g -O0 -I$(PKG_BUILD_DIR) -o $(PKG_BUILD_DIR)/lib/curl_helpers.o -c $(PKG_BUILD_DIR)/lib/curl_helpers.c
# 		$(TARGET_CC) $(TARGET_CFLAGS) -g -O0 -I$(PKG_BUILD_DIR) -o $(PKG_BUILD_DIR)/lib/key_pair.o -c $(PKG_BUILD_DIR)/lib/key_pair.c
# 		$(TARGET_CC) $(TARGET_CFLAGS) -g -O0 -I$(PKG_BUILD_DIR) -o $(PKG_BUILD_DIR)/lib/requests.o -c $(PKG_BUILD_DIR)/lib/requests.c
# 		$(TARGET_CC) $(TARGET_CFLAGS) -g -O0 -I$(PKG_BUILD_DIR) -o $(PKG_BUILD_DIR)/lib/scheduler.o -c $(PKG_BUILD_DIR)/lib/scheduler.c
# 		$(TARGET_CC) $(TARGET_CFLAGS) -g -O0 -I$(PKG_BUILD_DIR) -o $(PKG_BUILD_DIR)/lib/script_runner.o -c $(PKG_BUILD_DIR)/lib/script_runner.c

# 		$(TARGET_CC) $(TARGET_LDFLAGS) \
# 			$(PKG_BUILD_DIR)/main.o \
# 			$(PKG_BUILD_DIR)/services/access.o \
# 			$(PKG_BUILD_DIR)/services/accounting.o \
# 			$(PKG_BUILD_DIR)/services/config.o \
# 			$(PKG_BUILD_DIR)/services/device_data.o \
# 			$(PKG_BUILD_DIR)/services/device_status.o \
# 			$(PKG_BUILD_DIR)/services/did-key.o \
# 			$(PKG_BUILD_DIR)/services/end_report.o \
# 			$(PKG_BUILD_DIR)/services/setup.o \
# 			$(PKG_BUILD_DIR)/lib/console.o \
# 			$(PKG_BUILD_DIR)/lib/curl_helpers.o \
# 			$(PKG_BUILD_DIR)/lib/key_pair.o \
# 			$(PKG_BUILD_DIR)/lib/requests.o \
# 			$(PKG_BUILD_DIR)/lib/scheduler.o \
# 			$(PKG_BUILD_DIR)/lib/script_runner.o \
# 			-o $(PKG_BUILD_DIR)/wayru-os-services \
# 			-lcurl -ljson-c -lssl -lcrypto
# endef

# Package install instructions
# - Create the required directories
# - Install main scripts in the /usr/bin directory
# - Install init scripts in the /etc/init.d directory
# - Install app files in the /etc/wayru-os-services directory
define Package/wayru-os-services/install
		$(INSTALL_DIR) $(1)/usr/bin
		$(INSTALL_DIR) $(1)/etc/init.d
		$(INSTALL_DIR) $(1)/etc/config
		$(INSTALL_DIR) $(1)/etc/wayru-os-services
		$(INSTALL_DIR) $(1)/etc/wayru-os-services/scripts
		$(INSTALL_DIR) $(1)/etc/wayru-os-services/data
		$(INSTALL_DIR) $(1)/etc/wayru-os-services/data/did-key

		$(INSTALL_BIN) $(PKG_BUILD_DIR)/wayru-os-services $(1)/usr/bin/

		$(INSTALL_BIN) $(SOURCE_DIR)/scripts/openwrt/wayru-os-services.init $(1)/etc/init.d/wayru-os-services
		
		$(INSTALL_CONF) $(SOURCE_DIR)/scripts/openwrt/wayru-os-services.config $(1)/etc/config/wayru-os-services
		$(INSTALL_BIN) $(SOURCE_DIR)/scripts/openwrt/wayru-os-services.config $(1)/etc/wayru-os-services/config.uci

		$(INSTALL_BIN) $(SOURCE_DIR)/scripts/openwrt/disable-default-wireless.sh $(1)/etc/wayru-os-services/scripts/
		$(INSTALL_BIN) $(SOURCE_DIR)/scripts/openwrt/get-uuid.sh $(1)/etc/wayru-os-services/scripts/
		$(INSTALL_BIN) $(SOURCE_DIR)/scripts/openwrt/get-mac.sh $(1)/etc/wayru-os-services/scripts/
		$(INSTALL_BIN) $(SOURCE_DIR)/scripts/openwrt/nds-clients.sh $(1)/etc/wayru-os-services/scripts/
		$(INSTALL_BIN) $(SOURCE_DIR)/scripts/openwrt/nds-deauth.sh $(1)/etc/wayru-os-services/scripts/
		$(INSTALL_BIN) $(SOURCE_DIR)/scripts/openwrt/get-public-ip.sh $(1)/etc/wayru-os-services/scripts/
		$(INSTALL_BIN) $(SOURCE_DIR)/scripts/openwrt/get-osname.sh $(1)/etc/wayru-os-services/scripts/
		$(INSTALL_BIN) $(SOURCE_DIR)/scripts/openwrt/sign_cert.sh $(1)/etc/wayru-os-services/scripts/
		$(INSTALL_BIN) $(SOURCE_DIR)/scripts/openwrt/retrieve-data.lua $(1)/etc/wayru-os-services/scripts/
		
		$(INSTALL_DATA) certificates/ca.crt $(1)/etc/wayru-os-services/data/ca.crt
		$(INSTALL_DATA) .env $(1)/etc/wayru-os-services/data/.env
		$(INSTALL_DATA) VERSION $(1)/etc/wayru-os-services/VERSION
endef

define Package/wayru-os-services/conffiles
/etc/config/wayru-os-services
endef

# This command is always the last, it uses the definitions and variables we give above in order to get the job done
$(eval $(call BuildPackage,wayru-os-services))
