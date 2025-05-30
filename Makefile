include $(TOPDIR)/rules.mk

# Name, version and release number
# The name and version of your package are used to define the variable to point to the build directory of your package: $(PKG_BUILD_DIR)
PKG_NAME:=wayru-os-services
PKG_VERSION:=$(shell cat VERSION)
PKG_RELEASE:=1

# This is a custom variable, used below
SOURCE_DIR:=source

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

# Override the prepare step to copy local files
define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./CMakeLists.txt $(PKG_BUILD_DIR)/
	$(CP) ./source $(PKG_BUILD_DIR)/
	$(CP) ./VERSION $(PKG_BUILD_DIR)/
endef

# Package definition; instructs on how and where our package will appear in the overall configuration menu ('make menuconfig')
define Package/wayru-os-services
  SECTION:=admin
  CATEGORY:=Administration
  TITLE:=Wayru config daemon and scripts
  DEPENDS:=+libcurl +libjson-c +libopenssl +libmosquitto-ssl
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

	$(INSTALL_BIN) $(PKG_BUILD_DIR)/wayru-os-services $(1)/usr/bin/
	$(INSTALL_BIN) $(SOURCE_DIR)/scripts/openwrt/wayru-os-services.init $(1)/etc/init.d/wayru-os-services
	$(INSTALL_CONF) $(SOURCE_DIR)/scripts/openwrt/wayru-os-services.config $(1)/etc/config/wayru-os-services
	$(INSTALL_BIN) $(SOURCE_DIR)/scripts/openwrt/wayru-os-services.config $(1)/etc/wayru-os-services/config.uci

	$(INSTALL_BIN) $(SOURCE_DIR)/scripts/openwrt/*.sh $(1)/etc/wayru-os-services/scripts/
	$(INSTALL_BIN) $(SOURCE_DIR)/scripts/openwrt/*.lua $(1)/etc/wayru-os-services/scripts/
	$(INSTALL_DATA) VERSION $(1)/etc/wayru-os-services/VERSION
endef

define Package/wayru-os-services/conffiles
/etc/config/wayru-os-services
endef

# This command is always the last, it uses the definitions and variables we give above in order to get the job done
$(eval $(call BuildPackage,wayru-os-services))
