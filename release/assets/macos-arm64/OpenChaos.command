#!/bin/bash
# Double-click this file to launch OpenChaos.
# On first launch, macOS may ask for confirmation — click Open.

cd "$(dirname "$0")"

# Remove macOS quarantine flag (blocks unsigned apps downloaded from the internet)
xattr -dr com.apple.quarantine OpenChaos 2>/dev/null

# Ensure the binary is executable
chmod +x OpenChaos

./OpenChaos
