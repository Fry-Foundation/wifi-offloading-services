include $(TOPDIR)/rules.mk

# Name, version and release number
# The name and version of your package are used to define the variable to point to the build directory of your package: $(PKG_BUILD_DIR)
PKG_NAME:=wayru-os-services
PKG_VERSION:=1.0
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
  DEPENDS:=+sudo +php8-cgi
endef

# Package description; a more verbose description on what our package does
define Package/wayru-os-services/description
  An application for wayru-os.
endef

# Package preparation instructions; create the build directory and copy the source code. 
# The last command is necessary to ensure our preparation instructions remain compatible with the patching system.
define Build/Prepare
		mkdir -p $(PKG_BUILD_DIR)
		cp $(SOURCE_DIR)/* $(PKG_BUILD_DIR)
		$(Build/Patch)
endef

# Package build instructions; invoke the target-specific compiler to first compile the source file, and then to link the file into the final executable
define Build/Compile
		$(TARGET_CC) $(TARGET_CFLAGS) -o $(PKG_BUILD_DIR)/main.o -c $(PKG_BUILD_DIR)/main.c
		$(TARGET_CC) $(TARGET_LDFLAGS) $(PKG_BUILD_DIR)/main.o -o $(PKG_BUILD_DIR)/wayru-os-services
endef

# Package install instructions
# - Create the required directories
# - Install main scripts in the /usr/bin directory
# - Install init scripts in the /etc/init.d directory
# - Install library files in the /usr/lib/wayru-os-services directory
# 
# @TODO: Install uci-defaults
# @TODO: Install configuration files in the /etc/config directory
define Package/wayru-os-services/install
		$(INSTALL_DIR) $(1)/usr/bin
		$(INSTALL_DIR) $(1)/etc/init.d
		$(INSTALL_DIR) $(1)/etc/uci-defaults
		$(INSTALL_DIR) $(1)/etc/config
		$(INSTALL_DIR) $(1)/usr/lib/wayru-os-services

		$(INSTALL_BIN) $(PKG_BUILD_DIR)/wayru-os-services $(1)/usr/bin/

		$(INSTALL_BIN) files/wayru-os-services $(1)/etc/init.d/wayru-os-services
		
		$(INSTALL_BIN) files/binauth-accounting.sh $(1)/usr/lib/wayru-os-services/
		$(INSTALL_BIN) files/update-accounting.sh $(1)/usr/lib/wayru-os-services/
		$(INSTALL_BIN) files/update-config.sh $(1)/usr/lib/wayru-os-services/
		$(INSTALL_BIN) files/get-mac-address.sh $(1)/usr/lib/wayru-os-services/
		$(INSTALL_BIN) files/get-id.sh $(1)/usr/lib/wayru-os-services/
endef

# This command is always the last, it uses the definitions and variables we give above in order to get the job done
$(eval $(call BuildPackage,wayru-os-services))
