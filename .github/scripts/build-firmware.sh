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

# Pass version to Python script via environment variable
export VERSION_TO_SET="${VERSION}"

python3 << 'PYTHON_SCRIPT'
import re
import sys
import os

version = os.environ.get('VERSION_TO_SET')
if not version:
    print("ERROR: VERSION_TO_SET environment variable not set")
    sys.exit(1)

# Read the file
with open('platformio.ini', 'r') as f:
    content = f.read()

print(f"Looking for FIRMWARE_VERSION to replace with: {version}")
print("\nCurrent FIRMWARE_VERSION line(s):")
for line in content.split('\n'):
    if 'FIRMWARE_VERSION' in line:
        print(f"  {repr(line)}")

# Try multiple patterns to find the version line
patterns = [
    (r'-DFIRMWARE_VERSION=\\"([^"\\]+)\\"', r'-DFIRMWARE_VERSION=\\"{}\\"'),  # -DFIRMWARE_VERSION=\"0.1.0\"
    (r'-DFIRMWARE_VERSION=\\\"([^"]+)\\\"', r'-DFIRMWARE_VERSION=\\\"{}\\\"'),  # Double escaped
    (r'-DFIRMWARE_VERSION="([^"]+)"', r'-DFIRMWARE_VERSION="{}"'),  # Plain quotes
]

current_version = None
pattern_used = None

for search_pattern, replace_template in patterns:
    match = re.search(search_pattern, content)
    if match:
        current_version = match.group(1)
        pattern_used = (search_pattern, replace_template)
        print(f"\n✓ Matched pattern: {search_pattern}")
        print(f"  Current version: {current_version}")
        break

if not current_version:
    print("\n❌ Could not find FIRMWARE_VERSION in any expected format")
    print("\nFull platformio.ini content:")
    print(content)
    sys.exit(1)

# Replace the version
search_pattern, replace_template = pattern_used
new_content = re.sub(search_pattern, replace_template.format(version), content)

# Write back
with open('platformio.ini', 'w') as f:
    f.write(new_content)

print(f"\n✓ File updated")

# Verify
with open('platformio.ini', 'r') as f:
    verify_content = f.read()

# Check if new version appears in the file
if version in verify_content and 'FIRMWARE_VERSION' in verify_content:
    print(f"✓ Version successfully updated to {version}")
    print(f"\nNew FIRMWARE_VERSION line:")
    for line in verify_content.split('\n'):
        if 'FIRMWARE_VERSION' in line:
            print(f"  {line}")
else:
    print("❌ Failed to verify version update")
    print("\nplatformio.ini after update:")
    print(verify_content)
    sys.exit(1)
PYTHON_SCRIPT

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