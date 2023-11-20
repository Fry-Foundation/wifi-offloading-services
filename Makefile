.PHONY: dev default

# Development target
dev:
	@$(MAKE) -f Makefile-dev.mk

# Default target
default:
	@$(MAKE) -f Makefile-openwrt.mk