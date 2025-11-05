SHELL := /bin/bash
COMPANY ?= "SynaptixAI"
MW ?= "RSCL"
PROJECT_NAME ?= "ad_dcl"
TARGET_PALTFORM ?= "thor"
TGZ_VER := 1.$$(date '+%Y%m%d-%H%M%S')-$$(git rev-parse --short=5 HEAD).$(PROJECT_NAME)-$(TARGET_PALTFORM)
CLEAN_VER := $$([[ -n $$(git status -s) ]] && echo '-dirty' || echo '-new')
JOBS ?= 8
RELEASE_CONF := release.conf.json

COLOR_RED := \033[1;31m
COLOR_GREEN := \033[1;32m
COLOR_YELLOW := \033[1;33m
COLOR_RESET := \033[0m

FORCE:

all: $(RELEASE_CONF)
	@mkdir -p build
	@echo ""
	@echo "###################################################"
	@echo "# CMake"
	@echo "###################################################"
	@echo ""
	@cd build && cmake ..
	@echo ""
	@echo "###################################################"
	@echo "# Build"
	@echo "###################################################"
	@echo ""
	@cd build && make --no-print-directory -j$(JOBS)

clean:
	@rm -r build
	@rm -f $(RELEASE_CONF)

# 生成 release.conf.json
$(RELEASE_CONF): FORCE
	@echo "Generating $@"
	@chmod +x resource/tar_extra/release_helper.sh
	@PROJECT_NAME="$(PROJECT_NAME)" TARGET_PALTFORM="$(TARGET_PALTFORM)" ./resource/tar_extra/release_helper.sh

tgz_all: $(RELEASE_CONF)
	@echo "###################################################"
	@echo "# Make tgz_all"
	@echo "###################################################"
	@tar -czvf $(PROJECT_NAME)-$(TARGET_PALTFORM).tar.gz *

tgz_thor: $(RELEASE_CONF)
	@echo "###################################################"
	@echo "# Make tgz_thor"
	@echo "###################################################"
	@mkdir -p build
	@cd build && cmake .. \
		-DCOMPANY="$(COMPANY)" \
		-DCMAKE_TOOLCHAIN_FILE=/home/cmake/toolchain.cmake \
		-DCMAKE_MW="$(MW)" \
		-DCMAKE_BUILD_TYPE="Release" \
		-DBUILD_THOR=ON \
		-DCMAKE_INSTALL_PREFIX="/opt/tmp/ad_shadowmode" \
		-DMODULE_VERSION="$(TGZ_VER)$(CLEAN_VER)"
	@cd build && make package --no-print-directory -j$(JOBS)
