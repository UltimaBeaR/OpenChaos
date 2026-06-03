RELEASE_DIR := release
DIST_DIR    := $(RELEASE_DIR)/dist

ifeq ($(UNAME_S),Darwin)
    ifeq ($(UNAME_M),arm64)
        PLATFORM := macos-arm64
    else
        PLATFORM := macos-x64
    endif
    RELEASE_ASSETS := $(RELEASE_DIR)/assets/macos-arm64
else ifeq ($(UNAME_S),Linux)
    PLATFORM := linux-x64
    RELEASE_ASSETS := $(RELEASE_DIR)/assets/linux-x64
else
    PLATFORM := windows-x64
    RELEASE_ASSETS := $(RELEASE_DIR)/assets/windows-x64
endif

# Windows builds use the x64-windows-static vcpkg triplet, so all libraries are
# linked into OpenChaos.exe — there are no DLLs to ship alongside it.

# Usage: make release-package VERSION=0.1.0
# Output: release/dist/OpenChaos-v<VERSION>-<platform>.zip
# Requires: make configure && make build-release (run beforehand)
release-package:
ifndef VERSION
	$(error Usage: make release-package VERSION=0.1.0)
endif
	@if [ ! -f "$(BUILD_DIR)/Release/$(EXE_NAME)" ]; then \
	  echo "ERROR: Release build not found. Run 'make build-release' first." >&2; \
	  exit 1; \
	fi
	@rm -rf "$(DIST_DIR)"
	@ARCHIVE="OpenChaos-v$(VERSION)-$(PLATFORM)"; \
	STAGING="$(DIST_DIR)/$$ARCHIVE"; \
	mkdir -p "$$STAGING"; \
	echo "Packaging $$ARCHIVE..."; \
	cp "$(BUILD_DIR)/Release/$(EXE_NAME)" "$$STAGING/"; \
	sed 's/{{VERSION}}/$(VERSION)/g' "$(RELEASE_ASSETS)/README.txt" > "$$STAGING/OpenChaos-readme.txt"; \
	for extra in "$(RELEASE_ASSETS)"/*.command "$(RELEASE_ASSETS)"/*.sh; do \
	  [ -f "$$extra" ] && cp "$$extra" "$$STAGING/"; \
	done; \
	if [ "$(PLATFORM)" = "windows-x64" ]; then \
	  powershell -NoProfile -Command "Compress-Archive -Path '$(DIST_DIR)/$$ARCHIVE' -DestinationPath '$(DIST_DIR)/$$ARCHIVE.zip' -Force" && rm -rf "$(DIST_DIR)/$$ARCHIVE"; \
	else \
	  cd "$(DIST_DIR)" && zip -r "$$ARCHIVE.zip" "$$ARCHIVE" && rm -rf "$$ARCHIVE"; \
	fi; \
	echo ""; \
	echo "Done: $(DIST_DIR)/$$ARCHIVE.zip"
