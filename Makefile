include $(TOPDIR)/rules.mk

# Name, version and release number
# The name and version of your package are used to define the variable to point to the build directory of your package: $(PKG_BUILD_DIR)
PKG_NAME:=wayru-os-services
PKG_VERSION:=$(shell cat VERSION)
PKG_RELEASE:=1

# This is a custom variable, used below
AGENT_DIR:=apps/agent

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

# Override the prepare step to copy local files
define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./CMakeLists.txt $(PKG_BUILD_DIR)/
	$(CP) ./apps $(PKG_BUILD_DIR)/
	$(CP) ./lib $(PKG_BUILD_DIR)/
	$(CP) ./VERSION $(PKG_BUILD_DIR)/
endef

# Package definition; instructs on how and where our package will appear in the overall configuration menu ('make menuconfig')
define Package/wayru-os-services
  SECTION:=admin
  CATEGORY:=Administration
  TITLE:=Wayru config daemon and scripts
  DEPENDS:=+libcurl +libjson-c +libopenssl +libmosquitto-ssl +libubus +libubox +libblobmsg-json +lua
endef

# Package description; a more verbose description on what our package does
define Package/wayru-os-services/description
  Services for access and accounting for wayru-os
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
	$(INSTALL_DIR) $(1)/etc/wayru-os-services/data/did-key

	# Install the three new binaries
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/agent $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/config $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/collector $(1)/usr/bin/

	# Create a symlink for backward compatibility
	ln -sf /usr/bin/agent $(1)/usr/bin/wayru-os-services

	# Install init script and config
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/apps/agent/scripts/openwrt/wayru-os-services.init $(1)/etc/init.d/wayru-os-services
	$(INSTALL_CONF) $(PKG_BUILD_DIR)/apps/agent/scripts/openwrt/wayru-os-services.config $(1)/etc/config/wayru-os-services
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/apps/agent/scripts/openwrt/wayru-os-services.config $(1)/etc/wayru-os-services/config.uci

	# Install all scripts from the openwrt directory
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/apps/agent/scripts/openwrt/*.sh $(1)/etc/wayru-os-services/scripts/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/apps/agent/scripts/openwrt/*.lua $(1)/etc/wayru-os-services/scripts/

	# Install VERSION file
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/VERSION $(1)/etc/wayru-os-services/VERSION
endef

define Package/wayru-os-services/conffiles
/etc/config/wayru-os-services
endef

# This command is always the last, it uses the definitions and variables we give above in order to get the job done
$(eval $(call BuildPackage,wayru-os-services))
