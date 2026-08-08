# apply_bms_patch.sh - Script to apply BMS Chromium patches

#!/bin/bash

# BMS Chromium Patch Application Script
# Version: 1.0.0

echo "╔═══════════════════════════════════════════════════════════════════╗"
echo "║           BMS Browser - Chromium Patch Application              ║"
echo "╚═══════════════════════════════════════════════════════════════════╝"

# Check if we're in the Chromium source directory
if [ ! -d "src" ]; then
    echo "❌ Error: Not in Chromium source directory"
    echo "Please run this script from the root of your Chromium checkout"
    exit 1
fi

# Check if patch file exists
if [ ! -f "bms_changes.patch" ]; then
    echo "❌ Error: bms_changes.patch not found"
    exit 1
fi

# Apply the patch
echo "📝 Applying BMS patches to Chromium..."
cd src

# Dry run first
echo "🔍 Checking patch (dry run)..."
git apply --check ../bms_changes.patch

if [ $? -ne 0 ]; then
    echo "❌ Patch check failed. Please resolve conflicts."
    exit 1
fi

# Apply patch
echo "✅ Patch check passed. Applying..."
git apply ../bms_changes.patch

if [ $? -eq 0 ]; then
    echo "✅ Patch applied successfully!"
else
    echo "❌ Failed to apply patch."
    exit 1
fi

echo "✅ BMS Chromium patches applied successfully!"
echo ""
echo "Next steps:"
echo "1. Run 'gn gen out/Default' to generate build files"
echo "2. Run 'ninja -C out/Default chrome' to build"
echo "3. Run 'out/Default/chrome' to launch BMS Browser"