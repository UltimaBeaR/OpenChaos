#!/bin/bash
# Launch OpenChaos from the game directory.

cd "$(dirname "$0")"

# Ensure the binary is executable
chmod +x OpenChaos

./OpenChaos "$@"
