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
	$(INSTALL_DIR) $(1)/etc/wayru-agent
	$(INSTALL_DIR) $(1)/etc/wayru-agent/scripts
	$(INSTALL_DIR) $(1)/etc/wayru-agent/data
	$(INSTALL_DIR) $(1)/etc/wayru-agent/data/did-key
	$(INSTALL_DIR) $(1)/etc/wayru-config
	$(INSTALL_DIR) $(1)/etc/wayru-config/scripts
	$(INSTALL_DIR) $(1)/etc/wayru-collector

	# Install the three new binaries
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/wayru-agent $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/wayru-config $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/wayru-collector $(1)/usr/bin/

	# Install wayru-config scripts
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/apps/config/scripts/openwrt/renderer_applier.uc $(1)/etc/wayru-config/scripts/

	# Install init script and config
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/apps/agent/scripts/openwrt/wayru-agent.init $(1)/etc/init.d/wayru-agent
	$(INSTALL_CONF) $(PKG_BUILD_DIR)/apps/agent/scripts/openwrt/wayru-agent.config $(1)/etc/config/wayru-agent

	# Install wayru-config init script and config
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/apps/config/scripts/openwrt/wayru-config.init $(1)/etc/init.d/wayru-config
	$(INSTALL_CONF) $(PKG_BUILD_DIR)/apps/config/scripts/openwrt/wayru-config.config $(1)/etc/config/wayru-config

	# Install collector init script and config
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/apps/collector/scripts/openwrt/wayru-collector.init $(1)/etc/init.d/wayru-collector
	$(INSTALL_CONF) $(PKG_BUILD_DIR)/apps/collector/scripts/openwrt/wayru-collector.config $(1)/etc/config/wayru-collector

	# Install all scripts from the openwrt directory
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/apps/agent/scripts/openwrt/*.sh $(1)/etc/wayru-agent/scripts/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/apps/agent/scripts/openwrt/*.lua $(1)/etc/wayru-agent/scripts/

	# Install wayru-config scripts
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/apps/config/scripts/openwrt/renderer_applier.uc $(1)/etc/wayru-config/scripts/

	# Install VERSION file
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/VERSION $(1)/etc/wayru-os-services/VERSION
endef

define Package/wayru-os-services/conffiles
/etc/config/wayru-agent
/etc/config/wayru-config
/etc/config/wayru-collector
endef

# This command is always the last, it uses the definitions and variables we give above in order to get the job done
$(eval $(call BuildPackage,wayru-os-services))
