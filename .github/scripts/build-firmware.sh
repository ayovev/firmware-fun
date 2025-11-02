#!/bin/bash

# Build and sign firmware
# Called by semantic-release with version as first argument

set -e

VERSION=$1

if [ -z "$VERSION" ]; then
  echo "Error: Version not provided"
  exit 1
fi

echo "========================================="
echo "Building Firmware v${VERSION}"
echo "========================================="

# Update FIRMWARE_VERSION in platformio.ini while preserving other build_flags
echo "Updating version in platformio.ini..."

# Use sed to find and replace the FIRMWARE_VERSION line
# This preserves all other build_flags like -Wall, -Wextra, etc.
sed -i "s/-DFIRMWARE_VERSION=\\\\\"[^\\\"]*\\\\\"/-DFIRMWARE_VERSION=\\\\\\\\\\\\\"${VERSION}\\\\\\\\\\\\\"/g" platformio.ini

# Verify the change
if grep -q "FIRMWARE_VERSION=\\\\\"${VERSION}\\\\\"" platformio.ini; then
  echo "✓ Version updated to ${VERSION} in platformio.ini"
else
  echo "❌ Failed to update version in platformio.ini"
  exit 1
fi

# Show the updated build_flags for verification
echo ""
echo "Updated build_flags:"
grep -A 5 "^build_flags" platformio.ini || true
echo ""

# Build firmware
echo ""
echo "Building firmware..."
pio run --environment esp32-s3

if [ ! -f .pio/build/esp32-s3/firmware.bin ]; then
  echo "Error: Build failed - firmware.bin not found"
  exit 1
fi

echo "✓ Firmware built successfully"

# Generate SHA256 checksum
echo ""
echo "Generating checksum..."
sha256sum .pio/build/esp32-s3/firmware.bin | cut -d' ' -f1 > firmware.sha256
CHECKSUM=$(cat firmware.sha256)
echo "✓ SHA256: ${CHECKSUM}"

# Get file size
SIZE=$(stat -c%s .pio/build/esp32-s3/firmware.bin 2>/dev/null || stat -f%z .pio/build/esp32-s3/firmware.bin)
echo "✓ Size: ${SIZE} bytes"

# Sign firmware
echo ""
echo "Signing firmware..."

if [ -z "$SIGNING_KEY" ]; then
  echo "Warning: SIGNING_KEY not set - skipping signature"
  touch .pio/build/esp32-s3/firmware.bin.sig
else
  # Write private key to temporary file
  echo "$SIGNING_KEY" > private_key.pem
  
  # Hash the firmware
  openssl dgst -sha256 -binary \
    -out .pio/build/esp32-s3/firmware.bin.hash \
    .pio/build/esp32-s3/firmware.bin
  
  # Sign the hash
  openssl rsautl -sign \
    -in .pio/build/esp32-s3/firmware.bin.hash \
    -inkey private_key.pem \
    -out .pio/build/esp32-s3/firmware.bin.sig
  
  # Remove private key
  rm -f private_key.pem .pio/build/esp32-s3/firmware.bin.hash
  
  echo "✓ Firmware signed"
fi

# Create release manifest
echo ""
echo "Creating manifest..."
cat > manifest.json << EOF
{
  "version": "${VERSION}",
  "release_date": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
  "firmware_url": "https://github.com/${GITHUB_REPOSITORY}/releases/download/v${VERSION}/firmware.bin",
  "signature_url": "https://github.com/${GITHUB_REPOSITORY}/releases/download/v${VERSION}/firmware.bin.sig",
  "sha256": "${CHECKSUM}",
  "size_bytes": ${SIZE},
  "min_version_required": "1.0.0",
  "changelog_url": "https://github.com/${GITHUB_REPOSITORY}/releases/tag/v${VERSION}"
}
EOF

echo "✓ Manifest created"

echo ""
echo "========================================="
echo "Build Complete!"
echo "========================================="
echo "Version: ${VERSION}"
echo "Size: ${SIZE} bytes"
echo "SHA256: ${CHECKSUM}"
echo ""

# List all release artifacts
echo "Release artifacts:"
ls -lh .pio/build/esp32-s3/firmware.bin
ls -lh .pio/build/esp32-s3/firmware.bin.sig
ls -lh manifest.json
ls -lh firmware.sha256