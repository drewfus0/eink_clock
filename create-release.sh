#!/usr/bin/env bash
# ==============================================================================
# Script to build firmware, bump version, and prepare GitHub Release
# Usage:
#   ./create-release.sh          (auto-increments patch version, e.g. 1.0.0 -> 1.0.1)
#   ./create-release.sh 1.1.0    (sets explicit version)
# ==============================================================================

set -e

# Ensure we are in the project root
if [ ! -f "platformio.ini" ]; then
    echo "Error: Please run this script from the project root (/home/drewfus/.icons/eink_clock)."
    exit 1
fi

CONFIG_FILE="include/config.h"
VERSION_JSON="version.json"

# 1. Read current version from config.h
CURRENT_VERSION=$(grep '#define FIRMWARE_VERSION' "$CONFIG_FILE" | cut -d '"' -f 2)

if [ -z "$CURRENT_VERSION" ]; then
    echo "Error: Could not find FIRMWARE_VERSION in $CONFIG_FILE"
    exit 1
fi

echo "Current version: v$CURRENT_VERSION"

# 2. Determine target version
if [ -n "$1" ]; then
    NEW_VERSION="$1"
    # Strip leading 'v' if provided
    NEW_VERSION="${NEW_VERSION#v}"
    echo "Using provided version: v$NEW_VERSION"
else
    IFS='.' read -r -a parts <<< "$CURRENT_VERSION"
    major=${parts[0]:-1}
    minor=${parts[1]:-0}
    patch=${parts[2]:-0}
    patch=$((patch + 1))
    NEW_VERSION="$major.$minor.$patch"
    echo "Auto-incrementing version to: v$NEW_VERSION"
fi

# 3. Update include/config.h
echo "Updating $CONFIG_FILE..."
sed -i "s/#define FIRMWARE_VERSION .*/#define FIRMWARE_VERSION              \"$NEW_VERSION\"/" "$CONFIG_FILE"

# 4. Update version.json
echo "Updating $VERSION_JSON..."
cat <<EOF > "$VERSION_JSON"
{
  "version": "$NEW_VERSION",
  "date": "$(date +%Y-%m-%d)",
  "notes": "Release v$NEW_VERSION",
  "url": "https://github.com/drewfus0/eink_clock/releases/download/v$NEW_VERSION/firmware.bin"
}
EOF

# 5. Build firmware using PlatformIO
echo "Building firmware with PlatformIO..."
pio run -e firebeetle2_esp32e

BIN_PATH=".pio/build/firebeetle2_esp32e/firmware.bin"
if [ ! -f "$BIN_PATH" ]; then
    echo "Error: Build output $BIN_PATH not found!"
    exit 1
fi

# 6. Copy firmware to release directory
mkdir -p release
cp "$BIN_PATH" release/firmware.bin
echo "Firmware copied to: release/firmware.bin"

# 7. Git commit and tag
echo ""
echo "================================================="
echo "Firmware v$NEW_VERSION built successfully!"
echo "================================================="
echo ""
echo "To publish this release to GitHub:"
echo "  git add include/config.h version.json"
echo "  git commit -m \"Release v$NEW_VERSION\""
echo "  git tag v$NEW_VERSION"
echo "  git push origin main --tags"
echo ""
echo "Then on GitHub (https://github.com/drewfus0/eink_clock/releases/new):"
echo "  1. Choose tag 'v$NEW_VERSION'"
echo "  2. Drag and drop 'release/firmware.bin' into the release assets"
echo "  3. Click 'Publish release'"
echo ""
echo "The e-paper clock will automatically detect and install v$NEW_VERSION"
echo "on its next 6-hour NTP sync or when you press the RESET button!"
echo "================================================="
