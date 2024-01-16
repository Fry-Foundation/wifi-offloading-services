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
  DEPENDS:=+libcurl +libjson-c
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
		$(TARGET_CC) $(TARGET_CFLAGS) -o $(PKG_BUILD_DIR)/main.o -c $(PKG_BUILD_DIR)/main.c
		$(TARGET_CC) $(TARGET_CFLAGS) -o $(PKG_BUILD_DIR)/services/init.o -c $(PKG_BUILD_DIR)/services/init.c
		$(TARGET_CC) $(TARGET_CFLAGS) -o $(PKG_BUILD_DIR)/services/scheduler.o -c $(PKG_BUILD_DIR)/services/scheduler.c
		$(TARGET_CC) $(TARGET_CFLAGS) -o $(PKG_BUILD_DIR)/services/access.o -c $(PKG_BUILD_DIR)/services/access.c
		$(TARGET_CC) $(TARGET_CFLAGS) -o $(PKG_BUILD_DIR)/services/setup.o -c $(PKG_BUILD_DIR)/services/setup.c
		$(TARGET_CC) $(TARGET_CFLAGS) -o $(PKG_BUILD_DIR)/services/accounting.o -c $(PKG_BUILD_DIR)/services/accounting.c
		$(TARGET_CC) $(TARGET_CFLAGS) -o $(PKG_BUILD_DIR)/store/config.o -c $(PKG_BUILD_DIR)/store/config.c
		$(TARGET_CC) $(TARGET_CFLAGS) -o $(PKG_BUILD_DIR)/store/state.o -c $(PKG_BUILD_DIR)/store/state.c
		$(TARGET_CC) $(TARGET_CFLAGS) -o $(PKG_BUILD_DIR)/utils/requests.o -c $(PKG_BUILD_DIR)/utils/requests.c
		$(TARGET_CC) $(TARGET_CFLAGS) -o $(PKG_BUILD_DIR)/utils/script_runner.o -c $(PKG_BUILD_DIR)/utils/script_runner.c

		$(TARGET_CC) $(TARGET_LDFLAGS) \
			$(PKG_BUILD_DIR)/main.o \
			$(PKG_BUILD_DIR)/services/init.o \
			$(PKG_BUILD_DIR)/services/scheduler.o \
			$(PKG_BUILD_DIR)/services/access.o \
			$(PKG_BUILD_DIR)/services/setup.o \
			$(PKG_BUILD_DIR)/services/accounting.o \
			$(PKG_BUILD_DIR)/store/config.o \
			$(PKG_BUILD_DIR)/store/state.o \
			$(PKG_BUILD_DIR)/utils/requests.o \
			$(PKG_BUILD_DIR)/utils/script_runner.o \
			-o $(PKG_BUILD_DIR)/wayru-os-services \
			-lcurl -ljson-c
endef

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
		$(INSTALL_DATA) VERSION $(1)/etc/wayru-os-services/VERSION
endef

define Package/wayru-os-services/conffiles
/etc/config/wayru-os-services
endef

# This command is always the last, it uses the definitions and variables we give above in order to get the job done
$(eval $(call BuildPackage,wayru-os-services))
